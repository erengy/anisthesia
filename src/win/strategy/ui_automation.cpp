/*
MIT License

Copyright (c) 2017 Eren Okka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <string>
#include <vector>

#include <windows.h>
#include <uiautomation.h>

#include "ui_automation.h"

#include "../util.h"

namespace anisthesia {
namespace win {

// Windows Automation API reference:
// https://msdn.microsoft.com/en-us/library/windows/desktop/ff486375.aspx

struct Options {
  long control_type_id = 0;
  std::vector<std::pair<long, bool>> properties;
};

// Commonly used interfaces
using Condition = IUIAutomationCondition;
using Element = IUIAutomationElement;
using ElementArray = IUIAutomationElementArray;
using ValuePattern = IUIAutomationValuePattern;

// The main interface that is used throughout this file. Must be initialized
// before it can be used for the first time.
ComInterface<IUIAutomation> ui_automation;

// Forward declaration
Condition* BuildControlCondition(Element& element, const Options& options);

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

Element* GetElementFromArray(ElementArray& element_array, int index) {
  Element* element = nullptr;
  element_array.GetElement(index, &element);
  return element;
}

int GetElementArrayLength(ElementArray& element_array) {
  int length = 0;
  element_array.get_Length(&length);
  return length;
}

Element* FindFirstControl(Element& parent, TreeScope scope,
                          const Options& options) {
  Element* element = nullptr;

  ComInterface<Condition> condition(BuildControlCondition(parent, options));
  if (condition)
    parent.FindFirst(scope, condition.get(), &element);

  return element;
}

ElementArray* FindAllControls(Element& parent, TreeScope scope,
                              const Options& options) {
  ElementArray* element_array = nullptr;

  ComInterface<Condition> condition(BuildControlCondition(parent, options));
  if (condition)
    parent.FindAll(scope, condition.get(), &element_array);

  return element_array;
}

////////////////////////////////////////////////////////////////////////////////

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

Condition* GetControlViewCondition() {
  Condition* condition = nullptr;
  ui_automation->get_ControlViewCondition(&condition);
  return condition;
};

Condition* CreatePropertyCondition(PROPERTYID property_id, VARIANT variant) {
  Condition* condition = nullptr;
  ui_automation->CreatePropertyCondition(property_id, variant, &condition);
  return condition;
};

Condition* CreateBoolCondition(PROPERTYID property_id, bool value) {
  VARIANT variant;
  variant.vt = VT_BOOL;
  variant.lVal = value ? VARIANT_TRUE : VARIANT_FALSE;
  return CreatePropertyCondition(property_id, variant);
}

Condition* CreateLongCondition(PROPERTYID property_id, long value) {
  VARIANT variant;
  variant.vt = VT_I4;
  variant.lVal = value;
  return CreatePropertyCondition(property_id, variant);
}

Condition* BuildControlCondition(Element& element, const Options& options) {
  std::vector<ComInterface<Condition>> scoped_conditions;

  auto add_scoped_condition = [&scoped_conditions](Condition* condition) {
    if (condition)
      scoped_conditions.push_back(ComInterface<Condition>(condition));
  };

  add_scoped_condition(GetControlViewCondition());
  add_scoped_condition(CreateLongCondition(
        UIA_ControlTypePropertyId, options.control_type_id));
  for (const auto& property : options.properties) {
    add_scoped_condition(CreateBoolCondition(property.first, property.second));
  }

  if (scoped_conditions.empty())
    return nullptr;

  std::vector<Condition*> conditions;
  for (const auto& condition : scoped_conditions) {
    conditions.push_back(condition.get());
  }

  Condition* condition = nullptr;

  ui_automation->CreateAndConditionFromNativeArray(
      conditions.data(), static_cast<int>(conditions.size()), &condition);

  return condition;
}

////////////////////////////////////////////////////////////////////////////////

bool FindAddressBar(Element& parent, std::wstring& address) {
  // Here we assume that the first edit control that fits our options is the
  // address bar (e.g. the "Omnibox" on Chrome, the "Awesome Bar" on Firefox).
  Options options;
  options.control_type_id = UIA_EditControlTypeId;
  options.properties = {
    {UIA_IsEnabledPropertyId, true},
    {UIA_IsKeyboardFocusablePropertyId, true},
    {UIA_IsValuePatternAvailablePropertyId, true},
    {UIA_ValueIsReadOnlyPropertyId, false},
  };
  ComInterface<Element> element(
      FindFirstControl(parent, TreeScope_Descendants, options));
  if (!element)
    return false;

  // This element is named differently on each web browser (e.g. "Address and
  // search bar" on Chrome, "Search or enter address" on Firefox). This name
  // can change depending on the browser language. However, we are only
  // interested in the element value, which usually gives us the URL of the
  // current page.
  address = GetElementValue(*element);

  return true;
}

bool EnumerateTabs(Element& parent, std::vector<std::wstring>& tabs) {
  Options options;
  options.properties = {
    {UIA_ValueIsReadOnlyPropertyId, true},
  };

  // Find the first tab control, which contains the tab items
  options.control_type_id = UIA_TabControlTypeId;
  ComInterface<Element> element_tab(
      FindFirstControl(parent, TreeScope_Descendants, options));
  if (!element_tab)
    return false;

  // Enumerate tab items
  // WARNING: Chrome's tab items are unavailable when its window is minimized
  options.control_type_id = UIA_TabItemControlTypeId;
  ComInterface<ElementArray> element_array_tabs(
      FindAllControls(*element_tab, TreeScope_Children, options));
  if (!element_array_tabs)
    return false;
  const auto length = GetElementArrayLength(*element_array_tabs);
  for (int index = 0; index < length; ++index) {
    // Releasing the element array does not release individual elements. Each
    // element must be released to avoid leaking memory.
    ComInterface<Element> element_tab_item(
        GetElementFromArray(*element_array_tabs, index));
    tabs.push_back(GetElementName(*element_tab_item));
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool GetWebBrowserInformation(HWND hwnd, web_browser_information_proc_t proc) {
  if (!InitializeUIAutomation())
    return false;

  ComInterface<Element> parent(GetElementFromHandle(hwnd));
  if (!parent)
    return false;

  const std::wstring title = GetElementName(*parent);
  if (!proc(WebBrowserInformationType::Title, title))
    return false;

  std::wstring address;
  if (!FindAddressBar(*parent, address) ||
      !proc(WebBrowserInformationType::Address, address)) {
    return false;
  }

  std::vector<std::wstring> tabs;
  EnumerateTabs(*parent, tabs);
  for (const auto& tab : tabs) {
    if (!proc(WebBrowserInformationType::Tab, tab))
      return false;
  }

  return true;
}

}  // namespace win
}  // namespace anisthesia
