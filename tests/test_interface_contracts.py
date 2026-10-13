import sys
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from interface_contracts import audit_sources, declarations, finding_key, type_spelling


class InterfaceContractTests(unittest.TestCase):
    def test_literals_comments_and_nested_calls_preserve_arity(self):
        report = audit_sources({'src/a.c': '''
void logValue(const char *label, int value) {}
int pair(int x, int y) { return x + y; }
void caller(void) {
    /* logValue(); */
    logValue("commas, parentheses()", pair(1, 2));
    logValue("missing value");
}
'''})
        arity = [item for item in report['findings'] if item['kind'] == 'call-arity']
        self.assertEqual(len(arity), 1)
        self.assertEqual(arity[0]['shape'], {'expected': 2, 'actual': 1})

    def test_prototypes_are_not_calls_and_void_is_not_unspecified(self):
        report = audit_sources({'src/a.c': '''
void ready(void);
void legacy();
void ready(void) {}
void legacy(int value) {}
'''})
        self.assertEqual([item['name'] for item in report['findings']], ['legacy'])
        self.assertEqual(report['findings'][0]['kind'], 'unspecified-parameters')

    def test_knr_parameter_types_and_omitted_argument(self):
        report = audit_sources({'src/a.c': '''
long field(index, scratch)
long index;
long scratch;
{ return index; }
long caller(void) { return field(1); }
'''})
        self.assertEqual(report['declarations'][0]['parameters'], ['long', 'long'])
        self.assertEqual({item['kind'] for item in report['findings']},
                         {'knr-definition', 'call-arity'})

    def test_return_width_signedness_and_pointer_types_are_distinct(self):
        report = audit_sources({
            'src/a.c': 'u16 query(u8 *record) { return 0; }',
            'src/b.c': 's32 query(s32 record);',
            'include/query.h': 'u16 query(u8 *record);',
        })
        issues = report['findings']
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues[0]['kind'], 'signature-mismatch')
        self.assertEqual(issues[0]['shape']['actual'], ('s32', ['s32']))

    def test_parameter_names_arrays_and_float_aliases(self):
        self.assertEqual(type_spelling('f32 matrix[4][3]'), type_spelling('float (*)[3]'))
        self.assertEqual(type_spelling('u8 *record'), type_spelling('u8 *'))
        self.assertEqual(type_spelling('f64 number'), type_spelling('double'))
        self.assertNotEqual(type_spelling('const u8 *text'), type_spelling('u8 *text'))

    def test_shared_header_coverage(self):
        sources = {'src/a.c': 'int read(int index) { return index; }',
                   'src/b.c': 'int read(int);'}
        self.assertEqual(audit_sources(sources)['findings'][0]['kind'], 'missing-shared-header')
        sources['include/read.h'] = 'int read(int index);'
        self.assertEqual(audit_sources(sources)['findings'], [])

    def test_implicit_calls_and_bare_callback_references_need_shared_headers(self):
        for use in ['void caller(void) { operation(); }', 'void (*callback)(void) = operation;']:
            report = audit_sources({'src/a.c': 'void operation(void) {}', 'src/b.c': use})
            self.assertEqual(report['findings'][0]['kind'], 'missing-shared-header')

    def test_callback_arrays_and_variadic_calls(self):
        report = audit_sources({'src/a.c': '''
int (*handlers[4])();
void output(const char *format, ...) {}
void caller(void) { output("%d,%d", 1, 2); }
'''})
        self.assertEqual([item['kind'] for item in report['findings']], ['unspecified-callback'])

    def test_review_keys_survive_line_changes_but_not_signature_changes(self):
        def issue(source):
            return audit_sources({'src/a.c': source})['findings'][0]
        original = issue('void legacy();')
        moved = issue('\n\nvoid legacy();')
        changed = issue('s32 legacy();')
        self.assertEqual(finding_key(original), finding_key(moved))
        self.assertNotEqual(finding_key(original), finding_key(changed))

    def test_function_pointer_parameter_and_structure_tag(self):
        records = declarations('void visit(struct Node *node, void (*callback)(int, int));', 'src/a.c')
        self.assertEqual(records[0]['parameters'], ['structNode*', 'void(*)(int,int)'])

    def test_check_rejects_unreviewed_stale_and_incomplete_exceptions(self):
        tool = Path(__file__).resolve().parents[1] / 'tools/interface_contracts.py'
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ['src', 'include', 'docs']:
                (root / name).mkdir()
            source = root / 'src/a.c'
            source.write_text('void legacy();')
            def check():
                return subprocess.run([sys.executable, str(tool), '--root', str(root), '--check'],
                                      capture_output=True, text=True).returncode
            self.assertNotEqual(check(), 0)
            exception = audit_sources({'src/a.c': source.read_text()})['findings'][0]
            ledger = root / 'docs/INTERFACE_CONTRACTS.json'
            ledger.write_text(json.dumps({'exceptions': [exception]}))
            self.assertNotEqual(check(), 0)
            exception.update(reason='Reviewed legacy ABI', next_evidence='Compare target call setup')
            ledger.write_text(json.dumps({'exceptions': [exception]}))
            self.assertEqual(check(), 0)
            source.write_text('void legacy(void);')
            self.assertNotEqual(check(), 0)

    def test_arity_exception_is_specific_to_the_calling_function(self):
        def key(caller):
            report = audit_sources({'src/a.c': 'void target(int x) {}\n'
                                    f'void {caller}(void) {{ target(1, 2); }}'})
            return finding_key(report['findings'][0])
        self.assertNotEqual(key('first'), key('second'))


if __name__ == '__main__':
    unittest.main()
