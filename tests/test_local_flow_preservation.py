"""Waiting natives require both immediate and deferred preservation evidence."""

import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.script_decoder.local_flow import local_preservation_contracts


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


class DeferredPreservationTests(unittest.TestCase):
    def test_waiting_natives_require_both_current_reviews(self):
        contracts = [('actorScriptStartActionAndWait', 'ACTION_WAIT_DEFERRED_PIN',
                      'deferred_action_wait_review_sha256', 2),
                     ('actorScriptStartLoopExitAndWait', 'LOOP_EXIT_DEFERRED_PIN',
                      'deferred_loop_exit_review_sha256', 3),
                     ('actorScriptStartInterruptibleAction', 'INTERRUPTIBLE_DEFERRED_PIN',
                      'deferred_interruptible_review_sha256', 5)]
        for name, pin, field, handler in contracts:
            with self.subTest(native=name), tempfile.TemporaryDirectory() as temporary:
                active = dict(function=name, scheduling='wait_for_action')
                deferred = dict(action_handler=handler, dependencies=[dict(file='synthetic.c', sha256='reviewed')])
                root = Path(temporary)
                (root / 'src/ovl').mkdir(parents=True)
                (root / 'src/ovl/script_vm.c').write_text('/* synthetic source */')
                with patch('tools.script_decoder.local_flow.VM_PRESERVATION_PINS', {}), patch(
                        'tools.script_decoder.local_flow.NATIVE_PRESERVATION_PINS', {name: digest(active)}), patch(
                        'tools.script_decoder.local_flow.' + pin, digest(deferred)):
                    reviews = dict(active={name: active}, active_deferred={name: deferred})
                    current = local_preservation_contracts(reviews, root)
                    self.assertEqual(current['preserved_native_functions'], [name])
                    self.assertEqual(current[field], digest(deferred))
                    for immediate, delayed in ((active, None), (active, dict(deferred, completion_stack_delta=99)),
                                                (dict(active, scheduling='unreviewed'), deferred), (None, deferred)):
                        with self.subTest(immediate=immediate, delayed=delayed):
                            report = local_preservation_contracts(
                                dict(active={name: immediate}, active_deferred={name: delayed}), root)
                            self.assertEqual(report['preserved_native_functions'], [])
                            self.assertEqual(report['stale_native_reviews'], [name])


if __name__ == '__main__':
    unittest.main()
