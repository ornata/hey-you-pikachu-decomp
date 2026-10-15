"""Connect bounded VM dictionary requests to retained vocabulary descriptors.

The native consumes one integer-converted ID, pushes integer zero and yields.
Neither submission nor a static resource association proves device upload,
runtime selection, delivery of a recognized word or branch reachability.
"""

from .stack import incoming_edges, recover_arguments
from .vocabulary import dictionary_names
from .name_catalog import name_dictionary_request
from .saved_dictionary import saved_dictionary_analysis
import re


def dictionary_requests(instructions, dictionaries, name_catalog=None):
    """Keep request expressions and link only supported literal IDs to descriptors."""
    descriptors = {row['dictionary_id']: row for row in dictionaries}
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    rows = []
    for index, instruction in enumerate(instructions):
        if instruction.get('native', {}).get('functions') != ['actorScriptRequestMicDictionary']:
            continue
        row = dict(instruction_offset=instruction['offset'], function='actorScriptRequestMicDictionary',
                   contract=dict(arguments=['dictionary_id'], consumption='immediate',
                                 stack_result=0, scheduling='yield'),
                   scope='Request expression and possible resource; no upload, selection or reachability claim.')
        try:
            expression = recover_arguments(instructions, index, 1, incoming=incoming,
                                           allow_float=True, allow_symbolic=True, frame_entries=frames)[0]
            row.update(status='recovered_arguments', dictionary_id_expression=expression)
            if expression['kind'] == 'variable':
                saved = saved_dictionary_analysis(instructions, instruction['offset'], expression)
                if saved is not None:
                    row['saved_local_analysis'] = saved
            selection = name_dictionary_request(instructions, index, name_catalog)
            if selection is not None:
                row['conditional_selection'] = selection
                row['dictionary_id_domain'] = selection['dictionary_id_domain']
            if expression['kind'] in ('integer', 'float') and -2147483648 <= expression['value'] < 2147483648:
                identifier = int(expression['value'])
                row['constant_dictionary_id'] = identifier
                descriptor = descriptors.get(identifier)
                if descriptor is not None:
                    row['dictionary'] = dict(dictionary_id=identifier, count=descriptor['count'],
                                             location=descriptor['location'])
                    if 'recovered_name' in descriptor:
                        row['dictionary']['recovered_name'] = descriptor['recovered_name']
                else:
                    row['unresolved_resource'] = 'ID is outside the reviewed descriptor prefix; validity is not established.'
        except ValueError as error:
            row.update(status='unresolved', reason=str(error))
        rows.append(row)
    return rows


def source_dictionary_requests(source, dictionaries):
    """Inventory written C calls, retaining nonliteral arguments as expressions."""
    names = {name: identifier for identifier, name in dictionary_names().items()}
    descriptors = {row['dictionary_id']: row for row in dictionaries}
    rows = []
    for file, code in sorted(source.items()):
        for match in re.finditer(r'\bmicRequestDictionary\s*\(', code):
            prefix = code[code.rfind('\n', 0, match.start()) + 1:match.start()]
            if re.search(r'\b(?:void|extern)\s*$', prefix):
                continue
            depth = 1
            for end in range(match.end(), len(code)):
                depth += (code[end] == '(') - (code[end] == ')')
                if depth == 0:
                    break
            else:
                continue
            expression = code[match.end():end].strip()
            row = dict(file=file, line=code.count('\n', 0, match.start()) + 1,
                       expression=expression, scope='Written request; no upload or execution claim.')
            identifier = names.get(expression)
            if re.fullmatch(r'-?(?:0[xX][0-9a-fA-F]+|\d+)', expression):
                identifier = int(expression, 16 if 'x' in expression.lower() else 10)
            if identifier is not None:
                row['constant_dictionary_id'] = identifier
                if identifier in descriptors:
                    descriptor = descriptors[identifier]
                    row['dictionary'] = dict(dictionary_id=identifier, count=descriptor['count'])
                    if 'recovered_name' in descriptor:
                        row['dictionary']['recovered_name'] = descriptor['recovered_name']
            rows.append(row)
    return rows


def attach_dictionary_usage(vocabulary, configurations, source_requests):
    """Attach static request sites and existing bindings without asserting ownership."""
    for descriptor in vocabulary.get('dictionaries', []):
        identifier = descriptor['dictionary_id']
        descriptor['script_requests'] = [
            dict(configuration=candidate['address'], section=candidate['section'],
                 listing=candidate['listing'], instruction_offset=request['instruction_offset'],
                 direct_creation_sites=candidate['direct_creation_sites'],
                 assigned_creation_sites=candidate['assigned_creation_sites'],
                 companion_scenario_sites=candidate['companion_scenario_sites'],
                 basis='literal' if 'constant_dictionary_id' in request else 'conditional_name_catalog_domain')
            for candidate in configurations for request in candidate['voice_dictionary_requests']
            if request.get('constant_dictionary_id') == identifier or identifier in request.get('dictionary_id_domain', [])]
        descriptor['source_requests'] = [row for row in source_requests if row.get('constant_dictionary_id') == identifier]
        descriptor['usage_scope'] = 'Static requests with configured source bindings; no scene ownership, upload or execution claim. Empty usage does not establish disuse.'
