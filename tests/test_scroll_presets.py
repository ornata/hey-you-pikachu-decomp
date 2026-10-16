"""Small authored resources exercise authentication without retained game text."""
import copy
import hashlib
import struct
import unittest
from contextlib import ExitStack
from unittest.mock import patch
from tools.script_decoder import scroll_presets as presets


class Image:
    def __init__(self):
        self.allowed = {'.scene_authored'}
        self.memory = {}
    def put(self, address, data):
        self.memory.update({address+i:v for i,v in enumerate(data)})
    def symbol(self, name):
        return (name, 100)
    def read_address(self, address, count):
        try:
            return bytes(self.memory[address+i] for i in range(count)), dict(address=address)
        except KeyError as error:
            raise ValueError('Unreadable authored address.') from error


class ScrollPresetTests(unittest.TestCase):
    def fixture(self, stack):
        image=Image();table=struct.pack('>2I',200,220);image.put(100,table)
        bindings=[]
        for index, address, pointer, content in [(0,200,400,b'abc'),(1,220,500,b'def')]:
            words=[pointer,0] if index==0 else [pointer,0x3f800000,0]
            view=struct.pack('>'+str(len(words))+'I',*words)
            image.put(address,view);image.put(pointer,content+b'\0')
            metadata=[dict(address=pointer,bytes=len(content),sha256=hashlib.sha256(content).hexdigest())]
            bindings.append(dict(preset=index,address=address,pointer_words=len(words),readable_strings=1,
                pointer_view_sha256=hashlib.sha256(view).hexdigest(),tail_words=words[1:],
                strings_aggregate_sha256=presets._digest(metadata)))
        for name,value in [('TABLE_ADDRESS',100),('TABLE_SHA256',hashlib.sha256(table).hexdigest()),('PRESET_BINDINGS',bindings)]:
            stack.enter_context(patch.object(presets,name,value))
        return image, bindings

    def test_metadata_counts_and_permission_restoration(self):
        with ExitStack() as stack:
            image,_=self.fixture(stack);allowed=image.allowed
            result=presets.scroll_board_presets(image)
            self.assertEqual(result['status'],'authenticated_scroll_board_presets')
            self.assertEqual([(r['nonnull_pointer_word_count'],r['reviewed_readable_c_string_count']) for r in result['records']],[(1,1),(2,1)])
            self.assertIs(image.allowed,allowed)
            self.assertNotIn('abc',str(result));self.assertNotIn('def',str(result))

    def test_pointer_and_content_mismatches_fail_closed(self):
        for address in [100,200,400]:
            with self.subTest(address=address),ExitStack() as stack:
                image,_=self.fixture(stack);allowed=image.allowed;image.memory[address]^=1
                result=presets.scroll_board_presets(image)
                self.assertEqual(result['status'],'unresolved');self.assertEqual(result['records'],[])
                self.assertIs(image.allowed,allowed)

    def test_nul_bound_and_unreadable_pointer(self):
        for limit in [2,2048]:
            with self.subTest(limit=limit),ExitStack() as stack:
                image,_=self.fixture(stack)
                stack.enter_context(patch.object(presets,'STRING_BYTE_LIMIT',limit))
                if limit==2048:del image.memory[401]
                self.assertEqual(presets.scroll_board_presets(image)['status'],'unresolved')

    def test_invalid_tail_and_incomplete_bindings_fail_closed(self):
        with ExitStack() as stack:
            image,bindings=self.fixture(stack)
            changed=copy.deepcopy(bindings);changed[1]['tail_words']=[0]
            with patch.object(presets,'PRESET_BINDINGS',changed):
                self.assertEqual(presets.scroll_board_presets(image)['status'],'unresolved')
            with patch.object(presets,'PRESET_BINDINGS',[]):
                self.assertEqual(presets.scroll_board_presets(image)['status'],'unresolved')


if __name__=='__main__':unittest.main()
