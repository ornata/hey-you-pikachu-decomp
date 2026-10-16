"""Authenticate retained scrolling-board pointer views without exporting text.

Readable C strings are data facts, not a proof of rendered lines, resource
allocation, future scrolling safety, or ownership of neighboring words.
"""
import hashlib
import json
import struct

TABLE_ADDRESS = 0x8012A6B0
TABLE_SHA256 = 'ea36571fa76b225a9f6aa34a21fb3a4cac0919097995a2d3f0dd0dc97c097ffd'
STRING_BYTE_LIMIT = 2048
PRESET_BINDINGS = [{'address': 2148705976,
  'pointer_view_sha256': '61f2f0fbb1b3f4cab34e901587a92e877cf8344975bf5d3b5de798b707503557',
  'pointer_words': 133,
  'preset': 0,
  'readable_strings': 132,
  'strings_aggregate_sha256': 'bfcc1eaec102030a7de3f75acff3ca43acb08c517363a1555a0490faf0ff41ac',
  'tail_words': [0]},
 {'address': 2148706508,
  'pointer_view_sha256': 'b2ce6163f43aa99b1e2972cd870bd66f9ba194c0376fde342a8f877ab394f7ad',
  'pointer_words': 5,
  'preset': 1,
  'readable_strings': 1,
  'strings_aggregate_sha256': '5fba60ad337b630a73074a5a0aee4f09ec6c11ce1a863e048a7744334e408a30',
  'tail_words': [1065353216, 1065353216, 1065353216, 0]}]


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def scroll_board_presets(image):
    """Return authenticated metadata for two borrowed retained pointer views.

    The image must be an original-ROM-authenticated LocalImage. Its section
    permissions are restored even when resource authentication fails.
    """
    scope = ('Retained borrowed pointer views and readable NUL-terminated C strings only. '
             'Counts do not establish array allocation, supported visible lines, generated pixels, '
             'resource readiness or future scrolling safety; neighboring float words are not strings.')
    result = dict(status='unresolved', records=[], scope=scope)
    allowed = image.allowed
    image.allowed = {'.boot', '.ovl'}
    try:
        if len(PRESET_BINDINGS) != 2 or [b['preset'] for b in PRESET_BINDINGS] != [0, 1]:
            raise ValueError('Incomplete reviewed preset bindings.')
        if image.symbol('gScrollBoardPresetTexts')[1] != TABLE_ADDRESS:
            raise ValueError('Preset table address differs.')
        raw, location = image.read_address(TABLE_ADDRESS, 8)
        if len(raw) != 8 or hashlib.sha256(raw).hexdigest() != TABLE_SHA256:
            raise ValueError('Preset table differs.')
        addresses = struct.unpack('>2I', raw)
        records = []
        for binding, address in zip(PRESET_BINDINGS, addresses):
            if address != binding['address']:
                raise ValueError('Preset view address differs.')
            count = binding['pointer_words']
            if type(count) is not int or not 1 <= count <= 133:
                raise ValueError('Unbounded preset pointer extent.')
            view, view_location = image.read_address(address, count * 4)
            if len(view) != count * 4 or hashlib.sha256(view).hexdigest() != binding['pointer_view_sha256']:
                raise ValueError('Preset pointer view differs.')
            words = struct.unpack('>' + str(count) + 'I', view)
            valid = binding['readable_strings']
            if type(valid) is not int or not 0 < valid < count:
                raise ValueError('Invalid readable-string extent.')
            if list(words[valid:]) != binding['tail_words'] or not words[-1] == 0 or any(v == 0 for v in words[:-1]):
                raise ValueError('Preset tail differs.')
            strings = []
            for pointer in words[:valid]:
                text = bytearray()
                for offset in range(STRING_BYTE_LIMIT):
                    byte, _ = image.read_address(pointer + offset, 1)
                    if len(byte) != 1:
                        raise ValueError('Short string read.')
                    if byte == b'\0':
                        break
                    text.extend(byte)
                else:
                    raise ValueError('String exceeds bounded NUL read.')
                strings.append(dict(address=pointer, bytes=len(text), sha256=hashlib.sha256(text).hexdigest()))
            if _digest(strings) != binding['strings_aggregate_sha256']:
                raise ValueError('Readable-string metadata aggregate differs.')
            records.append(dict(preset=binding['preset'], pointer_view_address=address,
                pointer_word_count=count, nonnull_pointer_word_count=count - 1,
                reviewed_readable_c_string_count=valid, pointer_view_sha256=binding['pointer_view_sha256'],
                string_metadata_aggregate_sha256=binding['strings_aggregate_sha256'],
                string_metadata=strings, remaining_words=list(words[valid:]), location=view_location,
                ownership='Borrowed retained view and encoded strings; neighboring words have no inferred ownership.',
                scope=scope))
        result.update(status='authenticated_scroll_board_presets', table_address=TABLE_ADDRESS,
                      table_sha256=TABLE_SHA256, table_location=location, records=records)
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        result['reason'] = str(error)
    finally:
        image.allowed = allowed
    return result
