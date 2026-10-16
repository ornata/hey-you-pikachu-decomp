"""Serialize authored vocabulary records and memberships for isolated tests.

Inputs are synthetic (category, selector) pairs and word-index lists. This helper
contains no retail vocabulary labels, original dictionary lists or ROM byte blobs.
"""

import struct


def authored_vocabulary(records, dictionary_words):
    """Return the minimal image and bank interfaces used by vocabulary_catalog."""
    words_address = 100
    dictionaries_address = words_address + 16 * len(records)
    word_bytes = b''.join(struct.pack('>IIii', 500+i, 600+i, category, selector)
                          for i, (category, selector) in enumerate(records))
    membership_bytes = {10000+i*1024: struct.pack('>' + str(len(words)) + 'i', *words)
                        for i, words in enumerate(dictionary_words)}
    descriptor_bytes = b''.join(struct.pack('>iI', len(words), 10000+i*1024)
                                for i, words in enumerate(dictionary_words))

    class Image:
        def symbol(self, name):
            return None, {'gVoiceWordDefinitions': words_address, 'gVoiceDictionaries': dictionaries_address}[name]

        def read_rom(self, start, size):
            return b'x' * size

    class Bank:
        start, end, base, size = 1, 2, 3, 1

        def __init__(self, image):
            pass

        def text(self, address):
            return dict(address=address, ascii='synthetic', bytes_hex='')

        def read(self, address, size):
            if address == words_address:
                data = word_bytes
            elif address == dictionaries_address:
                data = descriptor_bytes
            elif address in membership_bytes:
                data = membership_bytes[address]
            else:
                data = b'\0' * size
            if len(data) != size:
                raise ValueError('synthetic fixture read has unexpected extent')
            return data, dict(address=address)

    return Image(), Bank
