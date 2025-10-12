#include <functional>
#include <string>
#include <vector>

#include <windows.h>
#include <unknwn.h>
#include <uiautomation.h>

#include <anisthesia/win_ui_automation.hpp>
#include <anisthesia/win_util.hpp>

namespace anisthesia::win::detail {

// Windows Automation API reference:
// https://msdn.microsoft.com/en-us/library/windows/desktop/ff486375.aspx

// Commonly used interfaces
using Element = IUIAutomationElement;
using TreeWalker = IUIAutomationTreeWalker;
using ValuePattern = IUIAutomationValuePattern;

using element_proc_t = std::function<TreeScope(Element&)>;
using properties_t = std::vector<std::pair<long, bool>>;

// The main interface that is used throughout this file. Must be initialized
// before it can be used for the first time.
ComInterface<IUIAutomation> ui_automation;

////////////////////////////////////////////////////////////////////////////////

bool InitializeUIAutomation() {
  if (ui_automation)
    return true;

  // COM library must be initialized on the current thread before calling
  // CoCreateInstance.
  ::CoInitialize(nullptr);

  IUIAutomation* ui_automation_interface = nullptr;
  const auto result = ::CoCreateInstance(
      CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_IUIAutomation,
      reinterpret_cast<void**>(&ui_automation_interface));
  ui_automation.reset(ui_automation_interface);

  return SUCCEEDED(result);
}

////////////////////////////////////////////////////////////////////////////////

Element* GetElementFromHandle(HWND hwnd) {
  Element* element = nullptr;
  ui_automation->ElementFromHandle(static_cast<UIA_HWND>(hwnd), &element);
  return element;
}

std::wstring GetElementName(Element& element) {
  std::wstring element_name;

  BSTR bstr = nullptr;
  if (SUCCEEDED(element.get_CurrentName(&bstr)) && bstr) {
    element_name = bstr;
    ::SysFreeString(bstr);
  }

  return element_name;
}

std::wstring GetElementValue(Element& element) {
  std::wstring element_value;

  ValuePattern* value_pattern_interface = nullptr;
  element.GetCurrentPatternAs(
      UIA_ValuePatternId, IID_PPV_ARGS(&value_pattern_interface));
  ComInterface<ValuePattern> value_pattern(value_pattern_interface);

  if (value_pattern) {
    BSTR bstr = nullptr;
    if (SUCCEEDED(value_pattern->get_CurrentValue(&bstr)) && bstr) {
      element_value = bstr;
      ::SysFreeString(bstr);
    }
  }

  return element_value;
}

////////////////////////////////////////////////////////////////////////////////

bool VerifyElementProperties(Element& element, const properties_t& properties) {
  VARIANT v = {};
  for (const auto& pair : properties) {
    if (FAILED(element.GetCurrentPropertyValue(pair.first, &v)))
      return false;
    if (v.boolVal != (pair.second ? VARIANT_TRUE : VARIANT_FALSE))
      return false;
  }

  return true;
}

bool IsAddressBarElement(Element& element) {
  static const properties_t properties = {
    {UIA_IsEnabledPropertyId, true},
    {UIA_IsKeyboardFocusablePropertyId, true},
    {UIA_IsValuePatternAvailablePropertyId, true},
    {UIA_ValueIsReadOnlyPropertyId, false},
  };

  return VerifyElementProperties(element, properties);
}

bool IsTabsElement(Element& element) {
  static const properties_t properties = {
    {UIA_ValueIsReadOnlyPropertyId, true},
  };

  return VerifyElementProperties(element, properties);
}

////////////////////////////////////////////////////////////////////////////////

void WalkElements(TreeWalker& tree_walker, Element& parent, TreeScope scope,
                  size_t depth, element_proc_t element_proc) {
  constexpr size_t kMaxTreeDepth = 16;  // arbitrary value
  if (depth > kMaxTreeDepth)
    return;

  if (scope & TreeScope_Element)
    scope = element_proc(parent);

  auto descend = [](TreeScope scope) {
    return (scope & TreeScope_Children) || (scope & TreeScope_Descendants);
  };

  if (descend(scope)) {
    Element* first_element = nullptr;
    tree_walker.GetFirstChildElement(&parent, &first_element);
    ComInterface<Element> element(first_element);

    while (element) {
      scope = element_proc(*element);

      if (descend(scope))
        WalkElements(tree_walker, *element, scope, depth + 1, element_proc);

      Element* next_element = nullptr;
      tree_walker.GetNextSiblingElement(element.get(), &next_element);
      element.reset(next_element);
    }
  }
}

bool FindWebBrowserElements(Element& parent, std::wstring& address,
                            std::vector<std::wstring>& tabs) {
  TreeWalker* tree_walker_interface = nullptr;
  ui_automation->get_ControlViewWalker(&tree_walker_interface);
  ComInterface<TreeWalker> tree_walker(tree_walker_interface);

  if (!tree_walker)
    return false;

  auto element_proc = [&](Element& element) -> TreeScope {
    CONTROLTYPEID control_type_id = 0;
    element.get_CurrentControlType(&control_type_id);

    switch (control_type_id) {
      default:
        // Are we done?
        if (!address.empty() && !tabs.empty())
          return TreeScope_Element;
        // Otherwise continue descending the tree.
        return TreeScope_Descendants;

      case UIA_DocumentControlTypeId:
      case UIA_MenuBarControlTypeId:
      case UIA_TitleBarControlTypeId:
        // We do not need to walk through these nodes. In fact, skipping
        // documents dramatically improves our performance on worst case
        // scenarios. This is the whole reason we are walking the tree rather
        // than using FindFirst and FindAll methods.
        return TreeScope_Element;

      case UIA_EditControlTypeId:
        // Here we assume that the first edit control that fits our properties
        // is the address bar (e.g. "Omnibox" on Chrome, "Awesome Bar" on
        // Firefox). This element is named differently on each web browser
        // (e.g. "Address and search bar" on Chrome, "Search or enter address"
        // on Firefox). This name can change depending on the browser
        // language. However, we are only interested in the element value,
        // which usually gives us the URL of the current page.
        if (address.empty() && IsAddressBarElement(element)) {
          address = GetElementValue(element);
          return TreeScope_Element;
        } else {
          // Opera has an edit control ("Address field") within another edit
          // control ("Address bar").
          return TreeScope_Descendants;
        }

      case UIA_TabControlTypeId:
        if (tabs.empty() && IsTabsElement(element))
          return TreeScope_Children;
        return TreeScope_Element;

      case UIA_TabItemControlTypeId:
        tabs.push_back(GetElementName(element));
        return TreeScope_Element;
    }
  };

  WalkElements(*tree_walker, parent, TreeScope_Subtree, 0, element_proc);
  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool GetWebBrowserInformation(HWND hwnd, web_browser_proc_t web_browser_proc) {
  if (!web_browser_proc)
    return false;

  if (!InitializeUIAutomation())
    return false;

  ComInterface<Element> parent(GetElementFromHandle(hwnd));
  if (!parent)
    return false;

  const std::wstring title = GetElementName(*parent);
  web_browser_proc({WebBrowserInformationType::Title, title});

  std::wstring address;
  std::vector<std::wstring> tabs;

  if (!FindWebBrowserElements(*parent, address, tabs))
    return false;

  web_browser_proc({WebBrowserInformationType::Address, address});
  for (const auto& tab : tabs) {
    web_browser_proc({WebBrowserInformationType::Tab, tab});
  }

  return true;
}

}  // namespace anisthesia::win::detail
