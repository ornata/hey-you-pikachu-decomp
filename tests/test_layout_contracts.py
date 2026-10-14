"""Exercise the lexical limits of the opaque record member inventory."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from layout_contracts import inspect_records, inventory, review_findings


class LayoutInventoryTests(unittest.TestCase):
    def test_comments_literals_and_locals_are_excluded(self):
        source = '''
/* struct Fake { s32 unknown00; }; */
char *text = "struct Fake { s32 unknown04; };";
void function(void) { s32 unknown08; }
typedef struct { s32 unknown0C; } Record;
'''
        findings = inspect_records(source, 'sample.c')
        self.assertEqual([(x['record'], x['member'], x['line']) for x in findings],
                         [('Record', 'unknown0C', 5)])

    def test_named_nested_union_pointer_and_array(self):
        source = '''struct Outer {
    union Inner { u32 unknown0; Record *unknown04; } view;
    const struct Other *unknown08;
    u8 unknown0C[2][4];
};'''
        findings = inspect_records(source, 'sample.h')
        self.assertEqual([x['record'] for x in findings],
                         ['Outer.Inner', 'Outer.Inner', 'Outer', 'Outer'])
        self.assertEqual([x['member'] for x in findings],
                         ['unknown0', 'unknown04', 'unknown08', 'unknown0C'])

    def test_repeated_members_are_distinct(self):
        findings = inspect_records('struct A { s32 unknown0; }; struct B { s32 unknown0; };', 'a.h')
        self.assertEqual([x['record'] for x in findings], ['A', 'B'])

    def test_known_fields_and_function_pointers_are_outside_scope(self):
        self.assertEqual(inspect_records('struct A { s32 count; void (*unknown0)(void); };', 'a.h'), [])

    def test_unclosed_record_is_reported(self):
        with self.assertRaisesRegex(ValueError, 'unclosed record'):
            inspect_records('struct A { s32 unknown0;', 'a.h')

    def test_sdk_manifest_excludes_supplied_records(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'docs').mkdir()
            for name in ['game', 'sdk']:
                (root / f'include/{name}.h').write_text('struct A { s32 unknown0; };')
            (root / 'docs/LOCAL_SDK_FILES.json').write_text(json.dumps({'files': [{'path': 'include/sdk.h'}]}))
            result = inventory(root)
            self.assertEqual((result['members'], result['files']), (1, 1))
            self.assertEqual(result['findings'][0]['file'], 'include/game.h')

    def test_initialized_anonymous_resource_has_stable_owner(self):
        source = 'struct { s32 unknown0; } resource = {0};'
        self.assertEqual(inspect_records(source, 'a.c')[0]['record'], 'resource')

    def review(self, finding):
        return dict(finding, known_facts='Four bytes after the header.', reason='Role unresolved.',
                    next_evidence='Trace initialization and readers.',
                    matching_constraint='Preserve width and offset.', evidence='a.h: declaration')

    def test_review_accepts_line_movement_but_rejects_declaration_changes(self):
        finding = inspect_records('struct A { s32 unknown0; };', 'a.h')[0]
        review = self.review(finding)
        moved = dict(finding, line=20)
        self.assertEqual(review_findings([moved], [review]), [])
        changed = dict(finding, declaration='u32 unknown0;')
        errors = review_findings([changed], [review])
        self.assertTrue(any('unreviewed' in item for item in errors))
        self.assertTrue(any('stale' in item for item in errors))

    def test_missing_and_duplicate_review_evidence_fails(self):
        finding = inspect_records('struct A { s32 unknown0; };', 'a.h')[0]
        review = self.review(finding)
        review['next_evidence'] = ' '
        errors = review_findings([finding], [review, review])
        self.assertTrue(any('duplicate' in item for item in errors))
        self.assertTrue(any('missing next_evidence' in item for item in errors))

    def test_new_and_removed_members_require_manual_review(self):
        finding = inspect_records('struct A { s32 unknown0; };', 'a.h')[0]
        self.assertTrue(any('unreviewed' in item for item in review_findings([finding], [])))
        self.assertTrue(any('stale' in item for item in review_findings([], [self.review(finding)])))
