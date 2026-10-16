"""Authored CFGs discriminate classifier roles without embedding species tables."""

import copy
import unittest
from unittest.mock import patch
from tools.script_decoder import fishing_record_helpers as helpers


class FishingRecordHelperTests(unittest.TestCase):
    def fixture(self,species=10,threshold=30):
        template,labels=helpers._template();start=1000;rows=[]
        names={'actorScriptPassThroughInteger':1,'actorScriptGetProgressValue':1,'actorScriptSetProgressValue':2,'actorScriptBuildSharedTextAlternate':'inline_count'}
        reviews=dict(active={name:dict(arguments=arity)for name,arity in names.items()})
        bindings={name:dict(arguments=arity,review_sha256=helpers._digest(reviews['active'][name]))for name,arity in names.items()}
        for spec in template:
            name=spec['name'];offset=start+spec['relative_offset'];operands=[]
            for i,value in enumerate(spec['args']):
                tag=2 if name in('JUMP','JUMP_IF_ZERO')or(name=='CALL_NATIVE'and i==0)else spec.get('literal_tag',0)
                if value in names:value=list(names).index(value)
                elif value=='species':value=species
                elif value=='threshold':value=threshold
                elif isinstance(value,str):value=start+labels[value]
                operands.append(dict(tag=tag,value=value))
            next_offset=offset+spec['size']
            if name=='RETURN':successors=[]
            elif name=='JUMP':successors=[dict(kind='branch',offset=operands[0]['value'])]
            elif name=='JUMP_IF_ZERO':successors=[dict(kind='fallthrough',offset=next_offset),dict(kind='branch',offset=operands[0]['value'])]
            else:successors=[dict(kind='resume_or_fallthrough'if name=='CALL_NATIVE'else'fallthrough',offset=next_offset)]
            row=dict(offset=offset,size=spec['size'],name='SCRIPT_OP_'+name,operands=operands,successors=successors)
            if name=='CALL_NATIVE':row['native']=dict(functions=[spec['args'][0]])
            rows.append(row)
        snapshot=(reviews,dict(stale_vm_functions=[],context_vm_contract=dict(current=True)))
        return rows,snapshot,bindings

    def recognize(self,rows,snapshot,bindings):
        with patch.dict(helpers.NATIVE_BINDINGS,bindings,clear=True):return helpers.fishing_record_helper_roles(rows,snapshot=snapshot)

    def test_full_branch_interface_preserves_clamping_and_unknown_units(self):
        rows,snapshot,bindings=self.fixture();result=self.recognize(rows,snapshot,bindings)[0]
        self.assertEqual(result['instruction_count'],63)
        self.assertEqual(result['definition_bytes'],455)
        self.assertEqual(result['tables']['species']['base_slot'],10)
        self.assertEqual(result['tables']['threshold']['base_slot'],30)
        self.assertEqual([r['expression']for r in result['possible_results']],['100 + species_index','species_index','-2','-1'])
        self.assertIn('clamps',result['effects'][0])
        self.assertIn('not independently proved',result['persistent_fields']['role'])
        self.assertEqual(result['indexed_source_role_sites'],[])

    def test_changed_branch_literal_table_coordinate_formal_and_inline_count_reject(self):
        rows,snapshot,bindings=self.fixture()
        for index,operand,value in [(0,0,1),(0,1,7),(2,1,3),(7,0,16),(9,0,1000),(15,0,139),(35,1,3),(42,0,99)]:
            bad=copy.deepcopy(rows);bad[index]['operands'][operand]['value']=value
            self.assertEqual(self.recognize(bad,snapshot,bindings),[],(index,operand))
        bad=copy.deepcopy(rows);bad[34]['operands'][1]['value']=31
        self.assertEqual(self.recognize(bad,snapshot,bindings),[])

    def test_indexed_site_is_separate_conditional_role_not_provenance(self):
        rows,snapshot,bindings=self.fixture();prefix=[];offset=0
        for name,values in [('LOAD_VARIABLE',[0,101]),('LOAD_ELEMENT',[0,112]),('STORE_VARIABLE',[1,41]),('LOAD_VARIABLE',[1,39]),('LOAD_VARIABLE',[1,41]),('CALL',[1000])]:
            size=1+5*len(values);tag=2 if name=='CALL'else 0
            successors=([dict(kind='call',offset=1000),dict(kind='return_continuation',offset=offset+size)]if name=='CALL'else[dict(kind='fallthrough',offset=offset+size)])
            prefix.append(dict(offset=offset,size=size,name='SCRIPT_OP_'+name,operands=[dict(tag=tag,value=v)for v in values],successors=successors));offset+=size
        prefix.append(dict(offset=offset,size=1,name='SCRIPT_OP_RETURN',operands=[],successors=[]))
        result=self.recognize(prefix+rows,snapshot,bindings)[0]
        site=result['indexed_source_role_sites'][0]
        self.assertEqual(site['element_source'],dict(frame=0,base_slot=112))
        self.assertEqual(site['status'],'conditional_role_observation_not_admitted_provenance')
        self.assertIn('initialization',site['scope'])
        prefix[2]['operands'][1]['value']=42
        self.assertEqual(self.recognize(prefix+rows,snapshot,bindings)[0]['indexed_source_role_sites'],[])

    def test_stale_each_native_vm_missing_and_ambiguous_native_fail_closed(self):
        for name in ('actorScriptPassThroughInteger','actorScriptGetProgressValue','actorScriptSetProgressValue','actorScriptBuildSharedTextAlternate'):
            rows,snapshot,bindings=self.fixture();snapshot[0]['active'][name]['changed']=True
            self.assertEqual(self.recognize(rows,snapshot,bindings),[])
        rows,snapshot,bindings=self.fixture();snapshot[1]['context_vm_contract']['current']=False
        self.assertEqual(self.recognize(rows,snapshot,bindings),[])
        rows,snapshot,bindings=self.fixture();rows[4]['native']['functions'].append('other')
        self.assertEqual(self.recognize(rows,snapshot,bindings),[])

    def test_competing_edges_duplicate_and_unknown_effect_reject(self):
        rows,snapshot,bindings=self.fixture()
        for issue in ('incoming','duplicate','unknown','successor'):
            bad=copy.deepcopy(rows)
            if issue=='incoming':bad.append(dict(offset=99999,size=1,name='SCRIPT_OP_JUMP',operands=[],successors=[dict(kind='branch',offset=bad[6]['offset'])]))
            if issue=='duplicate':bad.append(copy.deepcopy(bad[6]))
            if issue=='unknown':bad[18]['name']='SCRIPT_OP_STORE_ELEMENT'
            if issue=='successor':bad[9]['successors']=bad[9]['successors'][:1]
            self.assertEqual(self.recognize(bad,snapshot,bindings),[],issue)


if __name__=='__main__':unittest.main()
