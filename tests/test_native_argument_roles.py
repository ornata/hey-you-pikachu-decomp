"""Argument roles preserve VM push order and revoke stale source-bound annotations."""

import copy
import unittest
from unittest.mock import patch
from tools.script_decoder import native_arguments as roles


class NativeArgumentRoleTests(unittest.TestCase):
    def fixture(self):
        review = dict(arguments=2, stack_delta=-1, result_tag='integer')
        digest = roles.review_digest(review)
        rule = dict(review_sha256=digest, arguments=[
            dict(name='handle', conversion='integer'), dict(name='allowance', conversion='float')])
        capture = dict(kind='caller_box_capture', source_reference=dict(frame=1, slot=2),
                       caller_origin=dict(epoch='caller_before_CALL', load_instruction_offset=42))
        context = dict(consumer_function='fixtureNative', consumer_review_sha256=digest,
                       raw_callee_arguments=[dict(kind='variable', frame=1, slot=0),
                                             dict(kind='variable', frame=1, slot=1)],
                       contextual_arguments=[capture, dict(kind='float', value=.5)])
        return review, rule, context

    def test_push_order_and_caller_capture_are_preserved(self):
        review, rule, context = self.fixture()
        before = copy.deepcopy(context)
        with patch.dict(roles.ROLE_BINDINGS, {'fixtureNative':rule}):
            roles.annotate_native_input_roles([context], dict(active={'fixtureNative':review}))
        annotation = context.pop('native_input_roles')
        self.assertEqual(context, before)
        self.assertEqual([r['name'] for r in annotation['arguments']], ['handle','allowance'])
        self.assertEqual(annotation['arguments'][0]['contextual_input'], before['contextual_arguments'][0])
        self.assertEqual(annotation['arguments'][0]['contextual_input']['caller_origin']['load_instruction_offset'],42)

    def test_stale_review_context_and_arity_revoke_old_annotations(self):
        for mutation in ('review', 'context', 'arity', 'binding', 'missing'):
            review, rule, context = self.fixture()
            context['native_input_roles'] = {'stale':True}
            active = {'fixtureNative':review}
            if mutation=='review':review['stack_delta']=0
            if mutation=='context':context['consumer_review_sha256']='wrong'
            if mutation=='arity':
                review['arguments']=1
                rule['review_sha256']=context['consumer_review_sha256']=roles.review_digest(review)
            if mutation=='binding':rule['review_sha256']='wrong'
            if mutation=='missing':active={}
            with self.subTest(mutation=mutation),patch.dict(roles.ROLE_BINDINGS, {'fixtureNative':rule}):
                roles.annotate_native_input_roles([context], dict(active=active))
                self.assertNotIn('native_input_roles', context)

    def test_named_arguments_are_ordered_by_reviewed_roles(self):
        review, rule, context = self.fixture()
        raw=context['raw_callee_arguments'];actual=context['contextual_arguments']
        context['raw_callee_arguments']={'allowance':raw[1],'handle':raw[0]}
        context['contextual_arguments']={'allowance':actual[1],'handle':actual[0]}
        with patch.dict(roles.ROLE_BINDINGS, {'fixtureNative':rule}):
            roles.annotate_native_input_roles([context], dict(active={'fixtureNative':review}))
            self.assertEqual(context['native_input_roles']['arguments'][0]['contextual_input'],actual[0])
            context['contextual_arguments']['wrong']=context['contextual_arguments'].pop('handle')
            roles.annotate_native_input_roles([context], dict(active={'fixtureNative':review}))
            self.assertNotIn('native_input_roles',context)


if __name__=='__main__':
    unittest.main()
