"""Exercise expression recovery with authored bytecode, without ROM assets."""

import struct
import unittest

from tools.script_decoder.formats import decode_vm
from tools.script_decoder.progress_fields import progress_field_requests
from tools.script_decoder.stack import recover_arguments

NAMES = {
    3: 'SCRIPT_OP_NEGATE', 5: 'SCRIPT_OP_ADD', 6: 'SCRIPT_OP_SUBTRACT',
    7: 'SCRIPT_OP_MULTIPLY', 8: 'SCRIPT_OP_DIVIDE', 9: 'SCRIPT_OP_REMAINDER',
    24: 'SCRIPT_OP_PUSH_LITERAL',
    25: 'SCRIPT_OP_LOAD_VARIABLE', 27: 'SCRIPT_OP_CALL_NATIVE', 35: 'SCRIPT_OP_FINISH',
}
NATIVES = {
    7: dict(address=0x8011C270, functions=['actorScriptPassThroughInteger'], inline_operands=[]),
    236: dict(address=0x801202F8, functions=['actorScriptSetProgressValue'], inline_operands=[]),
    237: dict(address=0x80120348, functions=['actorScriptGetProgressValue'], inline_operands=[]),
    238: dict(address=0x80120388, functions=['actorScriptGetProgressMaximum'], inline_operands=[]),
    239: dict(address=0x801203C8, functions=['actorScriptGetProgressMinimum'], inline_operands=[]),
    888: dict(address=0x8011C98C, functions=['actorScriptCreateAttachedEffect'], inline_operands=[]),
    999: dict(address=0x80000000, functions=['unreviewedCommand'], inline_operands=[]),
}


def value(number, tag=0):
    payload = struct.pack('>f', number) if tag == 1 else struct.pack('>i', number)
    return bytes([tag]) + payload


def literal(number, tag=0):
    return bytes([24]) + value(number, tag)


def variable(frame, slot):
    return bytes([25]) + value(frame) + value(slot)


def native(selector):
    return bytes([27]) + value(selector, 2)


def listing(*parts):
    report = decode_vm(b''.join(parts) + bytes([35]), NAMES, natives=NATIVES)
    assert not report['diagnostics'], report['diagnostics']
    return report['instructions'][:-1]


def arguments(rows, **options):
    return recover_arguments(rows, len(rows) - 1, 2, **options)


class ScriptArgumentRecoveryTests(unittest.TestCase):
    def test_product_links_selector_without_clamping_the_requested_value(self):
        rows = listing(literal(134), literal(5), literal(60), bytes([7]), native(236))
        catalog = dict(fields=[dict(field_id=134, enum_name='BEST_SECONDS')], area_selectors=[])
        request = progress_field_requests(rows, catalog)[0]
        self.assertEqual(request['constant_selector'], 134)
        self.assertEqual(request['value_expression']['value'], 300)
        self.assertEqual(request['value_expression']['expression']['operator'], 'integer_multiply')
        self.assertEqual(request['field']['enum_name'], 'BEST_SECONDS')

    def test_unknown_value_conversion_does_not_hide_the_independent_selector(self):
        rows = listing(literal(117), variable(-1, 37), native(7), native(236))
        selector, converted = arguments(rows, allow_symbolic=True)
        self.assertEqual(selector['value'], 117)
        self.assertEqual(converted['kind'], 'integer_conversion')
        self.assertEqual(converted['operand'], dict(kind='variable', frame=-1, slot=37))
        self.assertNotIn('value', converted)
        with self.assertRaises(ValueError):
            arguments(rows)

    def test_integer_conversion_retains_signed_edges_and_truncates_fractions(self):
        for number, tag, expected in [(-2147483648, 0, -2147483648),
                                      (2147483647, 2, 2147483647),
                                      (-7.75, 1, -7), (7.75, 1, 7), (-0.0, 1, 0),
                                      (2147483520.0, 1, 2147483520)]:
            with self.subTest(number=number, tag=tag):
                rows = listing(literal(117), literal(number, tag), native(7), native(236))
                converted = arguments(rows, allow_float=True)[1]
                self.assertEqual((converted['kind'], converted['value']), ('integer', expected))
                self.assertEqual(converted['expression']['operator'], 'to_integer')

    def test_invalid_float_and_string_conversions_stop_recovery(self):
        for number, tag in [(2147483648.0, 1), (float('inf'), 1),
                            (float('nan'), 1), (3, 3), (3, 4)]:
            with self.subTest(number=number, tag=tag):
                rows = listing(literal(117), literal(number, tag), native(7), native(236))
                with self.assertRaises(ValueError):
                    arguments(rows, allow_float=True, allow_symbolic=True)

    def test_multiply_preserves_signedness_and_rejects_overflow(self):
        for left, right, expected in [(-5, 60, -300), (-5, -60, 300),
                                      (0, 2147483647, 0), (-2147483648, 1, -2147483648),
                                      (1073741823, 2, 2147483646)]:
            rows = listing(literal(134), literal(left), literal(right), bytes([7]), native(236))
            self.assertEqual(arguments(rows)[1]['value'], expected)
        for left, right in [(2147483647, 2), (-2147483648, -1)]:
            rows = listing(literal(134), literal(left), literal(right), bytes([7]), native(236))
            with self.assertRaises(ValueError):
                arguments(rows, allow_symbolic=True)

    def test_float_or_unknown_multiplication_stays_symbolic(self):
        for source in [literal(1.25, 1), variable(-1, 37)]:
            rows = listing(literal(134), source, literal(60), bytes([7]), native(236))
            product = arguments(rows, allow_float=True, allow_symbolic=True)[1]
            self.assertEqual((product['kind'], product['operator']), ('vm_operator', 'multiply'))
            self.assertNotIn('value', product)
            with self.assertRaises(ValueError):
                arguments(rows, allow_float=True)

    def test_division_and_remainder_use_truncation_toward_zero(self):
        for left, right, quotient, remainder in [(7, 3, 2, 1), (-7, 3, -2, -1),
                                                  (7, -3, -2, 1), (-7, -3, 2, -1),
                                                  (-2147483648, 1, -2147483648, 0)]:
            for opcode, expected in [(8, quotient), (9, remainder)]:
                rows = listing(literal(155), literal(left), literal(right), bytes([opcode]), native(236))
                self.assertEqual(arguments(rows)[1]['value'], expected)
        for left, right in [(7, 0), (-2147483648, -1)]:
            for opcode in (8, 9):
                rows = listing(literal(155), literal(left), literal(right), bytes([opcode]), native(236))
                with self.assertRaisesRegex(ValueError, 'outside the supported signed-32 domain'):
                    arguments(rows, allow_symbolic=True)

    def test_saved_value_stays_unknown_in_a_halving_request(self):
        rows = listing(literal(155), literal(155), native(237), literal(2), bytes([8]), native(236))
        request = progress_field_requests(rows, dict(fields=[dict(field_id=155)], area_selectors=[]))[-1]
        self.assertEqual(request['constant_selector'], 155)
        result = request['value_expression']
        self.assertEqual((result['kind'], result['operator']), ('vm_operator', 'divide'))
        self.assertEqual(result['operands'][0]['function'], 'actorScriptGetProgressValue')
        self.assertEqual(result['operands'][0]['arguments'][0]['value'], 155)
        self.assertNotIn('value', result['operands'][0])
        self.assertNotIn('value', result)

    def test_progress_getter_identity_and_area_selector_are_preserved(self):
        for selector, function in [(237, 'actorScriptGetProgressValue'),
                                   (238, 'actorScriptGetProgressMaximum'),
                                   (239, 'actorScriptGetProgressMinimum')]:
            rows = listing(literal(155), literal(0x40000001), native(selector), native(236))
            queried = arguments(rows, allow_symbolic=True)[1]
            self.assertEqual(queried['function'], function)
            self.assertEqual(queried['result_tag'], 'integer')
            self.assertEqual(queried['arguments'][0]['value'], 0x40000001)
            self.assertNotIn('value', queried)

    def test_unknown_native_or_competing_entry_stops_recovery(self):
        rows = listing(literal(117), literal(3), native(999), native(236))
        with self.assertRaises(ValueError):
            arguments(rows, allow_symbolic=True)
        rows = listing(literal(117), literal(3), native(7), native(236))
        incoming = {rows[i]['offset']: {rows[i - 1]['offset']} for i in range(1, len(rows))}
        incoming[rows[2]['offset']].add(1000)
        with self.assertRaisesRegex(ValueError, 'Another control-flow edge'):
            arguments(rows, incoming=incoming, allow_symbolic=True)

    def test_nested_conversions_keep_the_producer_budget(self):
        rows = listing(literal(117), literal(3), *[native(7) for _ in range(30)], native(236))
        self.assertEqual(arguments(rows)[1]['value'], 3)
        rows = listing(literal(117), literal(3), *[native(7) for _ in range(31)], native(236))
        with self.assertRaisesRegex(ValueError, 'bounded producer window'):
            arguments(rows)


class IntegerAnimationProgramDecodingTests(unittest.TestCase):
    modes = {0: 'CONSTANT', 1: 'RANGE', 2: 'EMIT', 3: 'JUMP'}

    class Image:
        def __init__(self, *keys):
            self.keys = {0x2000 + 12 * i: struct.pack('>ifhh', *key)
                         for i, key in enumerate(keys)}

        def read_address(self, address, size):
            if size != 12 or address not in self.keys:
                raise ValueError('outside synthetic key window')
            return self.keys[address], dict(address=address, rom_offset=address, section='.synthetic')

    def test_negative_duration_overrides_one_shot_mode_with_hold(self):
        from tools.script_decoder.effects import integer_key_program
        result = integer_key_program(self.Image((2, -1, -9, 123)), 0x2000, self.modes)
        self.assertEqual(result['status'], 'held')
        self.assertEqual(result['keys'][0]['operation'], 'hold')
        self.assertEqual(result['keys'][0]['start'], -9)

    def test_signed_relative_jumps_retain_a_loop_at_the_read_budget(self):
        from tools.script_decoder.effects import integer_key_program
        image = self.Image((3, -1, 1, 0), (0, 1, 5, 0), (3, -1, -1, 0))
        result = integer_key_program(image, 0x2000, self.modes, limit=3)
        self.assertEqual(result['status'], 'loop')
        self.assertEqual(result['loop_target'], 0x200C)
        self.assertEqual(result['keys'][2]['target_address'], 0x200C)

    def test_range_emission_and_hold_are_distinct_static_operations(self):
        from tools.script_decoder.effects import integer_key_program
        image = self.Image((1, 1, 0, 3), (2, 0, 99, 0), (0, -1, -9, 0))
        result = integer_key_program(image, 0x2000, self.modes)
        self.assertEqual(result['status'], 'held')
        self.assertEqual([r['operation'] for r in result['keys']],
                         ['inclusive_range', 'one_shot', 'hold'])
        self.assertEqual(result['keys'][1]['next_address'], 0x2018)

    def test_missing_keys_invalid_arithmetic_and_budget_stay_unresolved(self):
        from tools.script_decoder.effects import integer_key_program
        for image in [self.Image((3, -1, -1, 0)), self.Image((1, 0, 0, 3)),
                      self.Image((0, float('nan'), 0, 0)), self.Image((3, 1, 0, 0))]:
            self.assertEqual(integer_key_program(image, 0x2000, self.modes)['status'], 'unresolved')
        result = integer_key_program(self.Image((0, 1, 7, 0)), 0x2000, self.modes, limit=1)
        self.assertEqual(result['status'], 'unresolved')
        with self.assertRaises(ValueError):
            integer_key_program(self.Image(), 0x2000, self.modes, limit=0)

    def test_effect_links_preserve_unknown_kind_and_unnamed_selectors(self):
        from tools.script_decoder.effects import attached_effect_requests
        catalog = dict(programs=[dict(selector=167, enum_name='SCRIBBLE')])
        rows = listing(literal(0), literal(167), native(888))
        result = attached_effect_requests(rows, catalog)[0]
        self.assertEqual(result['status'], 'recovered_arguments')
        self.assertEqual(result['constant_effect_kind'], 0)
        self.assertEqual(result['effect_program']['enum_name'], 'SCRIBBLE')
        result = attached_effect_requests(listing(literal(167), native(888)), catalog)[0]
        self.assertEqual(result['status'], 'recovered_track_only')
        self.assertNotIn('effect_kind_expression', result)
        result = attached_effect_requests(listing(literal(0), literal(168), native(888)), catalog)[0]
        self.assertEqual(result['constant_track_selector'], 168)
        self.assertNotIn('effect_program', result)


class MotionDefinitionMetadataTests(unittest.TestCase):
    def test_named_bits_leave_unrecovered_bits_explicit(self):
        from tools.script_decoder.actions import motion_definition_metadata
        attributes = {7: 'ACTION_ATTRIBUTE_PLAYBACK_MODE_MASK', 8: 'ACTION_ATTRIBUTE_LOOP',
                      0x40000: 'ACTION_ATTRIBUTE_REGISTER_POSE_TRANSITION'}
        raw = bytearray(44)
        struct.pack_into('>IhBBh', raw, 12, 0x80040009, -1, 2, 3, -1)
        result = motion_definition_metadata(raw, attributes, {1: 'MOTION_PLAYBACK_TRACKS'})
        self.assertEqual(result['playback_mode_name'], 'MOTION_PLAYBACK_TRACKS')
        self.assertEqual(result['attribute_names'],
                         ['ACTION_ATTRIBUTE_LOOP', 'ACTION_ATTRIBUTE_REGISTER_POSE_TRANSITION'])
        self.assertEqual(result['unresolved_attribute_bits'], 0x80000000)
        self.assertEqual(result['motion_archive_id'], -1)
        self.assertEqual(result['initial_pose_id'], 2)
        self.assertEqual(result['final_pose_id'], 3)
        self.assertEqual(result['continuation_action_id'], -1)

    def test_unknown_modes_and_invalid_record_domains_are_not_guessed(self):
        from tools.script_decoder.actions import motion_definition_metadata
        attributes = {7: 'ACTION_ATTRIBUTE_PLAYBACK_MODE_MASK', 8: 'ACTION_ATTRIBUTE_LOOP'}
        raw = bytearray(44)
        struct.pack_into('>I', raw, 12, 7)
        self.assertIsNone(motion_definition_metadata(raw, attributes, {1: 'TRACKS'})['playback_mode_name'])
        with self.assertRaises(ValueError):
            motion_definition_metadata(raw[:-1], attributes, {})
        with self.assertRaises(ValueError):
            motion_definition_metadata(raw, {3: 'ACTION_ATTRIBUTE_PLAYBACK_MODE_MASK'}, {})
        with self.assertRaises(ValueError):
            motion_definition_metadata(raw, attributes | {24: 'INVALID_COMPOSITE'}, {})


    def test_meter_policy_is_a_field_and_motion_identity_is_not_an_archive_id(self):
        from tools.script_decoder.actions import motion_definition_metadata
        attributes = {7: 'ACTION_ATTRIBUTE_PLAYBACK_MODE_MASK', 0xC0000000: 'ACTION_ATTRIBUTE_METER_POLICY_MASK'}
        policies = {0: 'RECOVER', 0x40000000: 'DRAIN', 0x80000000: 'HOLD', 0xC0000000: 'RECOVER_FAST'}
        raw = bytearray(44)
        struct.pack_into('>i', raw, 0, 190)
        struct.pack_into('>IhBBh', raw, 12, 0xC0000001, 36, 0, 0, 198)
        result = motion_definition_metadata(raw, attributes, {1: 'TRACKS'}, policies,
                                            identities={190: 'TURN', 198: 'WALK'})
        self.assertEqual(result['attribute_names'], [])
        self.assertEqual(result['action_meter_policy_name'], 'RECOVER_FAST')
        self.assertEqual(result['unresolved_attribute_bits'], 0)
        self.assertEqual(result['motion_action_name'], 'TURN')
        self.assertEqual(result['continuation_action_name'], 'WALK')
        self.assertEqual(result['motion_archive_id'], 36)

    def test_projected_status_preserves_unrecovered_bits_and_action_scope(self):
        from tools.script_decoder.actions import script_action_flag_projection
        names = {16: 'SCRIPT_ACTION_HIGH_UPWARD_MOTION_LIMIT', 32: 'SCRIPT_ACTION_BYPASS_TERRAIN_AND_GRAVITY'}
        result = script_action_flag_projection(0x20000200, names)
        self.assertEqual(result['flags'], 0x30)
        self.assertEqual(result['unresolved_flag_bits'], 0)
        result = script_action_flag_projection(0x800, names)
        self.assertEqual(result['flags'], 0x40)
        self.assertEqual(result['known_names'], [])
        self.assertEqual(result['unresolved_flag_bits'], 0x40)
        self.assertEqual(script_action_flag_projection(0x810, names)['flags'], 0x42)


class PoseTransitionMapTests(unittest.TestCase):
    def test_explicit_precedence_and_first_automatic_pair_win(self):
        from tools.script_decoder.transitions import reconstruct_pose_map
        definitions = [dict(id=10, attributes=0x40000, initial_pose=1, final_pose=2),
                       dict(id=11, attributes=0x40000, initial_pose=1, final_pose=2),
                       dict(id=12, attributes=0x40000, initial_pose=2, final_pose=3),
                       dict(id=13, attributes=0x40000, initial_pose=2, final_pose=3),
                       dict(id=14, attributes=0x40000, initial_pose=4, final_pose=4),
                       dict(id=15, attributes=0, initial_pose=4, final_pose=5)]
        result = reconstruct_pose_map([dict(source_pose=1, destination_pose=2, action_id=-3)], definitions)
        self.assertEqual({r['key']: r['action_id'] for r in result['entries']}, {258: -3, 515: 12})
        self.assertEqual([r['status'] for r in result['candidates']],
                         ['inserted', 'duplicate', 'duplicate', 'inserted', 'duplicate'])
        self.assertEqual(result['candidates'][0]['transition_kind'], 'direct_apply')
        self.assertFalse(result['unresolved'])

    def test_capacity_probe_cycles_and_zero_values_remain_explicit(self):
        from tools.script_decoder.transitions import reconstruct_pose_map
        rows = [dict(source_pose=0, destination_pose=0, action_id=1),
                dict(source_pose=0, destination_pose=8, action_id=2)]
        result = reconstruct_pose_map(rows, [], capacity=2)
        self.assertEqual([r['status'] for r in result['candidates']], ['inserted', 'unreachable_free_slot'])
        self.assertTrue(result['unresolved'])
        result = reconstruct_pose_map([dict(source_pose=1, destination_pose=2, action_id=-256)], [])
        self.assertEqual(result['candidates'][0]['status'], 'unresolved_zero_value')
        with self.assertRaises(ValueError):
            reconstruct_pose_map([], [], capacity=0)

    def test_sentinel_only_table_does_not_read_neighboring_records(self):
        from tools.script_decoder.transitions import pose_transition_catalog
        class Image:
            allowed = set()
            def symbol(self, name):
                return name, 0x1000 if name == 'gActionExplicitPoseTransitions' else 0x2000
            def read_address(self, address, size):
                if address == 0x1000 and size == 4:
                    return struct.pack('>BBh', 48, 48, -3), dict(address=address)
                if 0x2000 <= address < 0x2000 + 330 * 44 and size == 44:
                    raw = bytearray(44)
                    struct.pack_into('>i', raw, 0, (address - 0x2000) // 44)
                    return bytes(raw), dict(address=address)
                raise AssertionError('neighboring explicit data must not be read')
        result = pose_transition_catalog(Image())
        self.assertEqual(result['explicit_records'], [])
        self.assertEqual(result['sentinel']['source_pose'], 48)
        self.assertEqual(result['entries'], [])

        class UnterminatedImage(Image):
            def read_address(self, address, size):
                if address in (0x1000, 0x1004):
                    return bytes(4), dict(address=address)
                return super().read_address(address, size)
        with self.assertRaises(ValueError):
            pose_transition_catalog(UnterminatedImage(), limit=2)


if __name__ == '__main__':
    unittest.main()
