###############################################################################################################################
#
# Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
import utils.composerutils


class Rule:
    def __init__(self, search_criteria={}, equivalent_properties=[]):
        self.search_criteria       = search_criteria
        self.equivalent_properties = equivalent_properties


class ElementRule:
    def __init__(self, match_element, rules=[]):
        self.match_element = match_element
        self.rules         = rules


assertions = {}


def assert_element_rules(element):
    """
    assert_element_rules

    Given an element, run assertions against that element

    Raises:
        CompositionException -- when equivalence assertion on element is violated
    """

    if element.tag not in assertions:
        return
    for assertion in assertions[element.tag]:
        for rule in assertion.rules:
            # Load in assertion rules and criteria
            match_element         = assertion.match_element
            search_criteria       = rule.search_criteria
            equivalent_properties = rule.equivalent_properties
            original_element = None
            for matched_element in list(element.findall(match_element)):
                # Ignore match_elements that do not meet search criteria
                if not search_match(matched_element, search_criteria):
                    continue
                # Ignore match_elements that are only defined once
                if not has_duplicates(element, match_element, search_criteria):
                    continue
                if original_element is None:
                    # Set the original element to compare against
                    original_element = matched_element
                else:
                    # Compare against original element
                    if not same_properties(original_element, matched_element, equivalent_properties):
                        detailed_str = get_details(element)
                        error_str_fmt = "Conflicting %s set for %s %s" % (matched_element.tag,
                                                                          element.tag,
                                                                          detailed_str)
                        raise utils.composerutils.CompositionException(error_str_fmt)


def get_details(element):
    """
    get_details

    Returns:
        String -- detailed information to help user find source of user error
    """
    detailed_str = ""
    if element.tag == "Node":
        node_name   = element.find("NodeName")
        instance_id = element.find("NodeInstanceId")
        if node_name is not None and instance_id is not None:
            detailed_str = "[%s (%s)]" % (node_name.text, instance_id.text)
    elif element.tag == "Link":
        src_port  = element.find('SrcPort/PortName')
        dst_ports = element.findall('DstPort/PortName')
        if src_port is not None and dst_ports is not None:
            detailed_str = "[%s]->[%s]" % (src_port.text, ",".join([dst.text for dst in dst_ports]))
    return detailed_str


def same_properties(original, other, equivalent_properties=[]):
    """
    same_properties

    Returns:
        Boolean -- when two elements have the same values for specified equivalent properties
    """

    props_to_compare = []
    if len(equivalent_properties) != 0:
        props_to_compare = equivalent_properties
    else:
        props_to_compare = [og.tag for og in original]
    for prop in props_to_compare:
        if len(original.findall(prop)) == 1:
            original_val = original.find(prop)
            other_val = other.find(prop)
            if original_val is None or other_val is None or original_val.text != other_val.text:
                return False
        else:
            # Multiple of same tag
            original_values = [og.text for og in original.findall(prop)]
            other_values = [ot.text for ot in other.findall(prop)]
            if set(original_values) != set(other_values):
                return False
    return True


def search_match(element, search_criteria=[]):
    """
    search_match

    Returns:
        Boolean -- Whether an element meets a search criteria
    """
    meets_search_criteria = True
    for key in search_criteria.keys():
        search_val = element.find(key)
        if search_val is None or search_val.text != search_criteria[key]:
            meets_search_criteria = False
            break
    return meets_search_criteria


def has_duplicates(element, match_element, search_criteria=[]):
    """
    has_duplicates

    Returns:
        Boolean -- Whether an element has duplicate match_elements that meet a search criteria
    """

    match_elements = []
    if len(element.findall(match_element)) <= 1:
        return False
    for child_element in element.findall(match_element):
        if search_match(child_element, search_criteria):
            match_elements.append(child_element)
    return len(match_elements) > 1


def define_assertions():
    """
    define_assertions

    Populates the assertions datastructure. Assertion keys are elements to run assertions on, and Assertion values
    are a list of ElementRules to enforce
    """

    # Node Assertions
    node_property_rules        = [Rule({"NodePropertyId": "2"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "4"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "5"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "6"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "7"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "8"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "9"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "10"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "11"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "12"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "13"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "14"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "15"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "16"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "17"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "18"}, ["NodePropertyValue"]),
                                  Rule({"NodePropertyId": "1023"}, ["NodePropertyValue"])]
    node_property_element_rule = ElementRule("NodeProperty", node_property_rules)
    assertions["Node"]         = [node_property_element_rule]

    # Link Assertions
    link_buffer_properties_rules        = [Rule()]
    link_buffer_properties_element_rule = ElementRule("BufferProperties", link_buffer_properties_rules)
    assertions["Link"]                  = [link_buffer_properties_element_rule]


# Run define_assertions() when module is loaded
define_assertions()