"""Authored helper shapes test interface recognition and fail-closed bindings."""

import copy
import unittest
from unittest.mock import patch
from tools.script_decoder import path_item_helpers as helpers


class PathItemHelperTests(unittest.TestCase):
    def fixture(self):
        specs=[('ENTER_FRAME',[1,4,1])]
        for axis in range(3):
            specs += [('PUSH_LITERAL',[0]),('LOAD_VARIABLE',[1,0]),('PUSH_LITERAL',[axis]),
                      ('CALL_NATIVE',[123]),('STORE_VARIABLE',[1,axis+1])]
        specs += [('LOAD_VARIABLE',[1,i])for i in (1,2,3)]
        specs += [('PUSH_LITERAL',[i])for i in (165,165,100,0,0)]
        specs += [('CALL_NATIVE',[125]),('RETURN',[])]
        rows=[];offset=100
        for name,values in specs:
            tag=2 if name=='CALL_NATIVE'else 0;size=1+5*len(values)
            row=dict(offset=offset,size=size,name='SCRIPT_OP_'+name,
                     operands=[dict(tag=tag,value=v)for v in values],
                     successors=[]if name=='RETURN'else[dict(kind='resume_or_fallthrough'if name=='CALL_NATIVE'else'fallthrough',offset=offset+size)])
            if name=='CALL_NATIVE':row['native']=dict(functions=['actorScriptGetAreaPathPointCoordinate'if values==[123]else'actorScriptFindItemNearPosition'])
            rows.append(row);offset+=size
        reviews=dict(active={'actorScriptGetAreaPathPointCoordinate':dict(arguments=3),
                             'actorScriptFindItemNearPosition':dict(arguments=8)})
        snapshot=(reviews,dict(stale_vm_functions=[],context_vm_contract=dict(current=True)))
        bindings={name:dict(arguments=r['arguments'],review_sha256=helpers._digest(r))for name,r in reviews['active'].items()}
        return rows,snapshot,bindings

    def recognize(self,rows,snapshot,bindings):
        with patch.dict(helpers.NATIVE_BINDINGS,bindings,clear=True):return helpers.path_item_helper_roles(rows,snapshot=snapshot)

    def test_exact_interface_is_not_item165_or_point_validation(self):
        rows,snapshot,bindings=self.fixture();result=self.recognize(rows,snapshot,bindings)[0]
        self.assertEqual(result['definition_bytes'],206)
        self.assertTrue(result['frame_contract']['local_count_includes_arguments'])
        self.assertEqual(result['frame_contract']['additional_local_cells'],3)
        self.assertEqual(result['query']['effective_kind_range'],[1,164])
        self.assertEqual(result['query']['distance_bound'],'inclusive')
        self.assertEqual(result['result']['tag'],0)
        self.assertIn('proves neither',result['preconditions'][1])
        self.assertEqual(result['instruction_evidence'][4]['native'],rows[4]['native'])

    def test_multiple_matching_entries_are_recognized_without_fixed_addresses(self):
        rows,snapshot,bindings=self.fixture();second=copy.deepcopy(rows)
        for row in second:
            row['offset']+=1000
            for edge in row['successors']:edge['offset']+=1000
        result=self.recognize(rows+second,snapshot,bindings)
        self.assertEqual([r['entry_offset']for r in result],[100,1100])

    def test_wrong_constants_axes_frame_formal_stores_boxing_or_size_reject(self):
        rows,snapshot,bindings=self.fixture()
        for index,operand,value in ((0,0,2),(0,1,5),(0,2,2),(1,0,1),(2,1,1),(3,0,2),(5,1,3),(19,0,164),(21,0,101),(22,0,6),(23,0,1)):
            bad=copy.deepcopy(rows);bad[index]['operands'][operand]['value']=value
            self.assertEqual(self.recognize(bad,snapshot,bindings),[],(index,operand))
        bad=copy.deepcopy(rows);bad[1]['operands'][0]['tag']=2
        self.assertEqual(self.recognize(bad,snapshot,bindings),[])
        bad=copy.deepcopy(rows);bad[1]['size']=7
        self.assertEqual(self.recognize(bad,snapshot,bindings),[])

    def test_stale_missing_ambiguous_reviews_and_vm_fail_closed(self):
        for issue in ('stale','missing','arity','vm','binding','native'):
            rows,snapshot,bindings=self.fixture()
            if issue=='stale':snapshot[0]['active']['actorScriptGetAreaPathPointCoordinate']['changed']=True
            if issue=='missing':snapshot[0]['active'].pop('actorScriptFindItemNearPosition')
            if issue=='arity':snapshot[0]['active']['actorScriptFindItemNearPosition']['arguments']=7
            if issue=='vm':snapshot[1]['context_vm_contract']['current']=False
            if issue=='binding':bindings.pop('actorScriptGetAreaPathPointCoordinate')
            if issue=='native':rows[4]['native']['functions'].append('other')
            self.assertEqual(self.recognize(rows,snapshot,bindings),[],issue)

    def test_branch_competing_entry_duplicate_offsets_and_wrong_return_reject(self):
        for issue in ('branch','incoming','duplicate','return'):
            rows,snapshot,bindings=self.fixture()
            if issue=='branch':rows[3]['successors'].append(dict(kind='branch',offset=rows[6]['offset']))
            if issue=='incoming':rows.append(dict(offset=999,size=1,name='SCRIPT_OP_JUMP',operands=[],successors=[dict(kind='branch',offset=rows[3]['offset'])]))
            if issue=='duplicate':rows.append(copy.deepcopy(rows[3]))
            if issue=='return':rows[-1]['successors']=[dict(kind='fallthrough',offset=999)]
            self.assertEqual(self.recognize(rows,snapshot,bindings),[],issue)


if __name__=='__main__':unittest.main()
