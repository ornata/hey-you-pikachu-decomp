"""Invented selectors guard literal bounds, grouped cases and unsupported expressions."""

import pytest

from tools.script_decoder.scenarios import scenario_domains


def selection():
    return '''s32 presentationSelectInvented(s32 *a, s32 *b) {
 if (flag) { return 2; }
 return 0;
}
void presentationInitializeForCurrentArea(void) {
 switch (gRuntimeCurrentArea) {
 case 0: case 1:
  gAreaScenarioIndex = presentationSelectInvented(&gCardPresentationActionScript, &gCardPresentationAudioCue); break;
''' + ''.join(f'case {i}: gAreaScenarioIndex = 0; break;\n' for i in range(2, 23)) + '''
 default: error(); break;
 }
}'''


def test_selector_indices_do_not_invent_intermediate_slots():
    domains = scenario_domains(selection())
    assert domains[0] == domains[1] == dict(indices=[0, 2], selector='presentationSelectInvented')
    assert domains[22] == dict(indices=[0], selector=None)
    named = selection().replace('case 22:', 'case AREA_INVENTED:')
    assert scenario_domains(named, {'AREA_INVENTED': 22}) == domains
    with pytest.raises(ValueError, match='unresolved'):
        scenario_domains(named)


def test_unknown_returns_duplicate_cases_and_incomplete_catalog_stop_inventory():
    for code in (selection().replace('return 2;', 'return other();'),
                 selection().replace('return 2;', 'return -1;'),
                 selection().replace('case 22:', 'case 21:'),
                 selection().replace('case 22: gAreaScenarioIndex = 0; break;', ''),
                 selection().replace('default: error(); break;', 'default: gAreaScenarioIndex = 99; break;'),
                 selection().replace('gAreaScenarioIndex = 0;', 'gAreaScenarioIndex = index;', 1)):
        with pytest.raises(ValueError):
            scenario_domains(code)
