"""Read voice metadata from the authenticated retail vocabulary DMA bank.

The original initializer copies ROM 0xA9800..0xB3C70 to 0x80100680.
This bank has no code-section address mapping. Table views and the reviewed
43-descriptor prefix are bounded evidence, not arbitrary dictionary-ID validity.
Original text stays in local decoder output and is never embedded in this tool.
"""

import hashlib
import json
import struct
import re

from .context import ROOT
from tools.source_quality import code_only


class VocabularyBank:
    """Resolve borrowed pointers only inside the reviewed original DMA span."""

    def __init__(self, image):
        self.image = image
        self.start = image.symbol('assets_a_ROM_START')[1]
        self.end = image.symbol('assets_a_ROM_END')[1]
        self.base = image.symbol('gVoiceVocabularyData')[1]
        if (self.start, self.end, self.base) != (0xA9800, 0xB3C70, 0x80100680):
            raise ValueError('vocabulary symbols disagree with the reviewed retail DMA mapping')
        self.size = self.end - self.start

    def read(self, address, size):
        """Read a positive span through the original loader's RAM-to-ROM mapping."""
        if size <= 0 or not self.base <= address < address + size <= self.base + self.size:
            raise ValueError('vocabulary reference lies outside the original DMA span')
        offset = self.start + address - self.base
        return self.image.read_rom(offset, size), dict(address=address, rom_offset=offset,
                                                      mapping='voiceRecognitionInitialize vocabulary DMA')

    def text(self, address, limit=256):
        """Retain encoded bytes and an ASCII display view when that view is valid."""
        if not address:
            raise ValueError('null vocabulary text reference')
        data = bytearray()
        for index in range(limit):
            byte, _ = self.read(address + index, 1)
            if byte == b'\0':
                try:
                    ascii_text = data.decode('ascii')
                except UnicodeDecodeError:
                    ascii_text = None
                return dict(address=address, bytes_hex=data.hex(), ascii=ascii_text)
            data.extend(byte)
        raise ValueError('vocabulary text exceeds the bounded read')


def phrase_names():
    """Read the scoped phrase selectors without conflating word and command IDs."""
    code = code_only((ROOT / 'include/voice_phrases.h').read_text())
    category = re.search(r'\bVOICE_PIKACHU_PHRASE_MESSAGE_TYPE\s*=\s*(\d+)', code)
    pairs = re.findall(r'\b(VOICE_PHRASE_\w+)\s*=\s*(\d+)', code)
    names = {int(value): name for name, value in pairs}
    if category is None or int(category[1]) != 2 or len(pairs) != 10 or set(names) != set(range(23, 33)):
        raise ValueError('phrase selectors disagree with the reviewed category/value domain')
    return names


# Reviewed names/value mapping only; never original vocabulary strings or word IDs.
GENERAL_COMMAND_MAPPING_SHA256 = 'a56b993efe2d8849a6fce460d44952dddb16c08f241dfa41991ec29a79cbaea1'


MAP_DESTINATION_MAPPING_SHA256 = '2598c44c63321e1007d1040e74167f807d9d15bc300b32581ac8b19dd0be1c89'


def _category_literal(code, identifier, expected):
    """Require one explicitly numbered category declaration, excluding expressions."""
    identifier = re.escape(identifier)
    categories = re.findall(r'\b' + identifier + r'\s*=\s*(0[xX][\da-fA-F]+|\d+)\s*(?=[,}])', code)
    if len(categories) != 1 or int(categories[0], 0) != expected or len(re.findall(r'\b' + identifier + r'\s*=', code)) != 1:
        raise ValueError('message category disagrees with the reviewed numeric domain')


def _reviewed_enum(code, enum_name, prefix, mapping_digest, *, excluded=()):
    """Parse a complete explicit enum and require its independently reviewed mapping."""
    enum = re.escape(enum_name)
    enums = re.findall(r'\btypedef\s+enum\s+' + enum + r'\s*\{([^}]+)\}\s*' + enum + r'\s*;', code)
    if len(enums) != 1 or len(re.findall(r'\benum\s+' + enum + r'\b', code)) != 1:
        raise ValueError('missing or duplicate reviewed selector enum')
    declarations = enums[0].strip().split(',')
    if declarations[-1].strip() == '':
        declarations.pop()
    names, identifiers = {}, set()
    for declaration in declarations:
        match = re.fullmatch(r'\s*(' + re.escape(prefix) + r'\w+)\s*=\s*(0[xX][\da-fA-F]+|\d+)\s*', declaration)
        if match is None:
            raise ValueError('malformed reviewed selector declaration')
        name, value = match[1], int(match[2], 0)
        if name in identifiers or value in names or not 0 <= value <= 2147483647 or value in excluded:
            raise ValueError('duplicate or unsupported reviewed selector')
        names[value] = name
        identifiers.add(name)
    digest = hashlib.sha256(json.dumps(names, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if not names or mapping_digest is None or digest != mapping_digest:
        raise ValueError('selector mapping needs renewed review')
    return names


def general_command_names():
    """Read the exact reviewed general-command enum without accepting aliases."""
    code = code_only((ROOT / 'include/voice_phrases.h').read_text())
    _category_literal(code, 'VOICE_GENERAL_COMMAND_MESSAGE_TYPE', 2)
    return _reviewed_enum(code, 'VoiceGeneralCommandValue', 'VOICE_GENERAL_COMMAND_',
                          GENERAL_COMMAND_MAPPING_SHA256, excluded=range(23, 33))


def map_destination_names():
    """Read authored voice destination selectors, separate from internal map indices."""
    code = code_only((ROOT / 'include/voice_map_destination.h').read_text())
    message_code = code_only((ROOT / 'include/script_message.h').read_text())
    _category_literal(message_code, 'SCRIPT_MESSAGE_WORLD_MAP_DESTINATION', 18)
    return _reviewed_enum(code, 'VoiceMapDestinationValue', 'VOICE_MAP_DESTINATION_',
                          MAP_DESTINATION_MAPPING_SHA256)


ITEM_NOUN_GROUP_MAPPING_SHA256 = '367f5ab198e0282721f5fabd0ff35959e455d97e4ea8da5ccf13f49c3c81d9da'


def item_noun_group_names():
    """Read scoped noun-group requests without inventing direct item-kind aliases."""
    code = code_only((ROOT / 'include/voice_item_selector.h').read_text())
    _category_literal(code, 'VOICE_ITEM_NOUN_MESSAGE_TYPE', 6)
    return _reviewed_enum(code, 'VoiceItemNounGroupValue', 'VOICE_ITEM_NOUN_GROUP_',
                          ITEM_NOUN_GROUP_MAPPING_SHA256)


def dictionary_names():
    """Read explicitly named selections while leaving other descriptors numeric."""
    code = code_only((ROOT / 'include/voice_recognition.h').read_text())
    pairs = re.findall(r'\b(VOICE_DICTIONARY_\w+)\s*=\s*(-?\d+)', code)
    names = {int(value): name for name, value in pairs}
    if names != {-1: 'VOICE_DICTIONARY_NONE', 0: 'VOICE_DICTIONARY_DEFAULT', 31: 'VOICE_DICTIONARY_WORLD_MAP'}:
        raise ValueError('named dictionary selections need renewed caller/resource review')
    return names


def vocabulary_catalog(image, dictionary_count=43, *, validate_phrases=True, validate_general_commands=True, validate_map_destinations=True, validate_item_noun_groups=True):
    """Link a reviewed descriptor prefix to bounded words and gameplay metadata."""
    if not 1 <= dictionary_count <= 256:
        raise ValueError('dictionary prefix bound must be between 1 and 256')
    bank = VocabularyBank(image)
    phrases = phrase_names() if validate_phrases else {}
    general_commands = general_command_names() if validate_general_commands else {}
    map_destinations = map_destination_names() if validate_map_destinations else {}
    item_noun_groups = item_noun_group_names() if validate_item_noun_groups else {}
    selections = dictionary_names()
    words_address = image.symbol('gVoiceWordDefinitions')[1]
    dictionaries_address = image.symbol('gVoiceDictionaries')[1]
    span = dictionaries_address - words_address
    if span <= 0 or span % 16 or span // 16 > 1024:
        raise ValueError('word definitions need a bounded aligned neighbor-delimited view')
    raw, word_location = bank.read(words_address, span)
    words = []
    for offset in range(0, span, 16):
        name, text, command_type, command_value = struct.unpack_from('>IIii', raw, offset)
        display = bank.text(name)
        row = dict(word_id=offset // 16, address=words_address + offset,
                   name=display, device_text=bank.text(text),
                   command_type=command_type, command_value=command_value,
                   dictionary_memberships=[])
        if command_type == 2 and command_value in phrases:
            recovered = phrases[command_value]
            expected = recovered.removeprefix('VOICE_PHRASE_').replace('_', '-').casefold()
            if display['ascii'] is None or display['ascii'].rstrip('-').casefold() != expected:
                raise ValueError('phrase selector disagrees with its retained vocabulary label')
            row['recovered_phrase_name'] = recovered
        if command_type == 2 and command_value in general_commands:
            row['recovered_general_command_name'] = general_commands[command_value]
        if command_type == 18 and command_value in map_destinations:
            row['recovered_map_destination_name'] = map_destinations[command_value]
        if command_type == 6 and command_value in item_noun_groups:
            row['recovered_item_noun_group_name'] = item_noun_groups[command_value]
        words.append(row)
    raw, dictionary_location = bank.read(dictionaries_address, dictionary_count * 8)
    dictionaries = []
    for identifier in range(dictionary_count):
        count, pointer = struct.unpack_from('>iI', raw, identifier * 8)
        if not 0 < count <= 256:
            raise ValueError('reviewed dictionary requires a positive bounded word count')
        identifiers, location = bank.read(pointer, count * 4)
        ids = list(struct.unpack('>' + str(count) + 'i', identifiers))
        if any(not 0 <= word < len(words) for word in ids):
            raise ValueError('dictionary word ID lies outside the bounded word view')
        for index, word in enumerate(ids):
            words[word]['dictionary_memberships'].append(dict(dictionary_id=identifier, entry_index=index))
        descriptor = dict(dictionary_id=identifier, count=count, word_ids=ids, location=location)
        if identifier in selections:
            descriptor['recovered_name'] = selections[identifier]
        dictionaries.append(descriptor)
    following, following_location = bank.read(dictionaries_address + dictionary_count * 8, 8)
    if phrases and {word['command_value'] for word in words if 'recovered_phrase_name' in word and word['dictionary_memberships']} != set(phrases):
        raise ValueError('named phrases need membership in the reviewed dictionary prefix')
    if general_commands and {word['command_value'] for word in words if 'recovered_general_command_name' in word
                             and any(member['dictionary_id'] < 43 for member in word['dictionary_memberships'])} != set(general_commands):
        raise ValueError('named general commands need membership in the reviewed43 dictionary prefix')
    if map_destinations and {word['command_value'] for word in words if 'recovered_map_destination_name' in word
                             and any(member['dictionary_id'] == 31 for member in word['dictionary_memberships'])} != set(map_destinations):
        raise ValueError('named map destinations need membership in reviewed dictionary31')
    if item_noun_groups and {word['command_value'] for word in words if 'recovered_item_noun_group_name' in word
                            and any(member['dictionary_id'] < 43 for member in word['dictionary_memberships'])} != set(item_noun_groups):
        raise ValueError('named item noun groups need membership in the reviewed43 dictionary prefix')
    return dict(mapping=dict(rom_start=bank.start, rom_end=bank.end, ram_start=bank.base,
                             sha256=hashlib.sha256(image.read_rom(bank.start, bank.size)).hexdigest(),
                             original_loader=0x800404E8),
                word_view=dict(**word_location, bytes=span, count=len(words),
                               scope='Neighbor-delimited record view; no complete allocation claim.'),
                dictionary_view=dict(**dictionary_location, count=dictionary_count,
                                     following_bytes_hex=following.hex(), following_location=following_location,
                                     scope='Explicitly reviewed descriptor prefix; following padding does not prove all valid request IDs.'),
                words=words, dictionaries=dictionaries,
                scope='Retained vocabulary metadata and memberships; no recognition success, selected dictionary or gameplay reachability claim.')
