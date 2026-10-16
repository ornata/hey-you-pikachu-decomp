"""Source-bound input roles for selected VM natives, in original push order.

Roles label boxed input positions. They establish neither converted values nor
resource validity, actor identity, message delivery or successful native effects.
"""

import hashlib
import json

# Frozen expanded reviews, renewed only after independent source/ROM review.
ROLE_BINDINGS = {'actorScriptAddProgressValue': {'arguments': [{'conversion': 'integer', 'name': 'field_id'},
                                               {'conversion': 'integer', 'name': 'delta'}],
                                 'review_sha256': '80bde8a0b27ed31afcc99e24c87731e566877c8094cc44773b76b9266e164811'},
 'actorScriptCreateItem': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                         {'conversion': 'float', 'name': 'y'},
                                         {'conversion': 'float', 'name': 'z'},
                                         {'conversion': 'float', 'name': 'pitch'},
                                         {'conversion': 'float', 'name': 'yaw'},
                                         {'conversion': 'integer', 'name': 'kind'}],
                           'review_sha256': '6340dd117531fe9dc5e51ca2dc2af04cca1ad2524f7e44ca8bf38b74f604af3f'},
 'actorScriptCreateStationaryItem': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                                   {'conversion': 'float', 'name': 'y'},
                                                   {'conversion': 'float', 'name': 'z'},
                                                   {'conversion': 'float', 'name': 'pitch'},
                                                   {'conversion': 'float', 'name': 'yaw'},
                                                   {'conversion': 'integer', 'name': 'kind'}],
                                     'review_sha256': '5180cc71d71908c576a318a6e9c67aac20410ed0d2f7a603758acccae8907d07'},
 'actorScriptFadePrimaryOverlayBlend': {'arguments': [{'conversion': 'integer', 'name': 'red'},
                                                      {'conversion': 'integer', 'name': 'green'},
                                                      {'conversion': 'integer', 'name': 'blue'},
                                                      {'conversion': 'integer', 'name': 'alpha'},
                                                      {'conversion': 'float', 'name': 'duration'}],
                                        'review_sha256': 'aa382706d241e081dd8a3eeec884057af86d479ad868c092273b3e65a82ecf09'},
 'actorScriptFindItemInFacingCone': {'arguments': [{'conversion': 'integer',
                                                    'name': 'minimum_kind'},
                                                   {'conversion': 'integer',
                                                    'name': 'maximum_kind'},
                                                   {'conversion': 'float',
                                                    'name': 'distance_limit'},
                                                   {'conversion': 'float',
                                                    'name': 'cone_angle_degrees'},
                                                   {'conversion': 'integer',
                                                    'name': 'item_state_filter'},
                                                   {'conversion': 'integer',
                                                    'name': 'weighted_preference'}],
                                     'review_sha256': 'b3afbcfd7baa5f84ab90afc84fe86a095cd52560f9fbc5250e216e7602bb830a'},
 'actorScriptFindItemNearPosition': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                                   {'conversion': 'float', 'name': 'y'},
                                                   {'conversion': 'float', 'name': 'z'},
                                                   {'conversion': 'integer',
                                                    'name': 'minimum_kind'},
                                                   {'conversion': 'integer',
                                                    'name': 'maximum_kind'},
                                                   {'conversion': 'float',
                                                    'name': 'maximum_distance'},
                                                   {'conversion': 'integer',
                                                    'name': 'ownership_filter'},
                                                   {'conversion': 'integer',
                                                    'name': 'weighted_preference'}],
                                     'review_sha256': '23430c21142012be426362cce69429a2432a3122c58fd53c0fd13b3d1af799ba'},
 'actorScriptFindNearestInFacingCone': {'arguments': [{'conversion': 'integer',
                                                       'name': 'minimum_kind'},
                                                      {'conversion': 'integer',
                                                       'name': 'maximum_kind'},
                                                      {'conversion': 'float',
                                                       'name': 'distance_limit'},
                                                      {'conversion': 'float',
                                                       'name': 'cone_angle_degrees'}],
                                        'review_sha256': 'a8391079eb66905e238edf7f8791ae4f19de4cee4f5e550719689c887afcd1e4'},
 'actorScriptFindObjectInFacingCone': {'arguments': [{'conversion': 'integer',
                                                      'name': 'minimum_kind'},
                                                     {'conversion': 'integer',
                                                      'name': 'maximum_kind'},
                                                     {'conversion': 'float',
                                                      'name': 'distance_limit'},
                                                     {'conversion': 'float',
                                                      'name': 'cone_angle_degrees'}],
                                       'review_sha256': '0fb2ccbb850a7de2801eaa8c97f380091ab8fb1bfcf7d56a50c7871b84ceeb1c'},
 'actorScriptGetActorKind': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                             'review_sha256': '70813e1f2e4de9c0150fb070184dcefaac134e3970a0392f91c1e2cebce2e2ce'},
 'actorScriptGetAreaPathPointCoordinate': {'arguments': [{'conversion': 'integer',
                                                          'name': 'path_index'},
                                                         {'conversion': 'integer',
                                                          'name': 'point_index'},
                                                         {'conversion': 'integer', 'name': 'axis'}],
                                           'review_sha256': '86b5998903dea69b9c71670ac35eef019d4ce209d0e670e6984b5e644644cad5'},
 'actorScriptGetBoundingRadius': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                  'review_sha256': '8a96ed33b614bb27637153940c452e3c92ee7598c2ada535dccf53cbd206c1e1'},
 'actorScriptGetCreatureActionFlags': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                       'review_sha256': 'b616c6415f51957202c9f3a08c18d15fd0cc05c950eb9fc59297d2a6e22f96f6'},
 'actorScriptGetItemCollectionValue': {'arguments': [{'conversion': 'integer',
                                                      'name': 'item_kind'}],
                                       'review_sha256': 'dc13ee95093b62a526b868233a8eb789c615c381ff837f1ef6ce5a9213b4e0e0'},
 'actorScriptGetItemKind': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                            'review_sha256': 'd641aeada0243298e7f97e94fcee69866b83d2724cd73235d7a864b92d231bb8'},
 'actorScriptGetItemState': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                             'review_sha256': '2b82e3bf0b97bc624ddee4b36f9d5838d388c29282969c92778c01213398f009'},
 'actorScriptGetObjectType': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                              'review_sha256': '18540a9de22e7fb8c6748ba98310c2931628b7e4669e6bd4adcff7edc88edcd1'},
 'actorScriptGetPokemonNameDictionaryGroup': {'arguments': [{'conversion': 'integer',
                                                             'name': 'catalog_index'}],
                                              'review_sha256': 'd0e54bbc7ae9267d93bf3a98832fe62709eaee042ecc67ad08bcf64ec9283e8c'},
 'actorScriptGetProgressValue': {'arguments': [{'conversion': 'integer', 'name': 'field_id'}],
                                 'review_sha256': '3d51ae69bf9ab291b45dfe4f839926f44f00eaa87325b8c5c55a0e8bf01c48df'},
 'actorScriptPassThroughInteger': {'arguments': [{'conversion': 'integer', 'name': 'value'}],
                                   'review_sha256': '7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8'},
 'actorScriptPostMessage': {'arguments': [{'conversion': 'integer', 'name': 'recipient_handle'},
                                          {'conversion': 'integer', 'name': 'message_type'},
                                          {'conversion': 'integer', 'name': 'argument0'},
                                          {'conversion': 'integer', 'name': 'argument2'},
                                          {'conversion': 'integer', 'name': 'argument3'},
                                          {'conversion': 'integer', 'name': 'argument4'}],
                            'review_sha256': '56d573988e3f8c6dec5e2ed2d69f84b1e936c5a6794a5b81d4acf154ba22ff3d'},
 'actorScriptSetDestination': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                             {'conversion': 'float', 'name': 'y'},
                                             {'conversion': 'float', 'name': 'z'},
                                             {'conversion': 'float', 'name': 'distance_allowance'}],
                               'review_sha256': '1c370b7746b0918f2de2f2a526637057b09f9deea902d80a881d386c5d0d4df9'},
 'actorScriptSetProgressValue': {'arguments': [{'conversion': 'integer', 'name': 'field_id'},
                                               {'conversion': 'integer',
                                                'name': 'requested_value'}],
                                 'review_sha256': '0139883166d83c2855045ef88d951316e7006dc43eea5168cb8dd228b7d2d6ff'},
 'actorScriptTrackObject': {'arguments': [{'conversion': 'integer', 'name': 'target_handle'},
                                          {'conversion': 'float', 'name': 'distance_allowance'}],
                            'review_sha256': 'eb681b97de95b273a4137c8707f2aef34ea1ab2ad8ab5758eee8af3fe98cca4c'}}


def review_digest(review):
    """Fingerprint the complete reviewed callback contract deterministically."""
    return hashlib.sha256(json.dumps(review, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def annotate_native_input_roles(contexts, reviews):
    """Add role metadata only to fresh generated contexts with exact frozen reviews.

    reviews must be a source-validated snapshot. Existing context arguments stay
    unchanged; stale role annotations are revoked before each attempted analysis.
    """
    for context in contexts:
        context.pop('native_input_roles', None)
        function = context.get('consumer_function')
        rule = ROLE_BINDINGS.get(function)
        review = reviews.get('active', {}).get(function)
        if rule is None or review is None:
            continue
        digest = review_digest(review)
        arguments = rule['arguments']
        if digest != rule['review_sha256'] or context.get('consumer_review_sha256') != digest:
            continue
        if type(review.get('arguments')) is not int or review['arguments'] != len(arguments):
            continue
        names = [row['name'] for row in arguments]
        def ordered(value):
            if isinstance(value, dict):
                return [value[name] for name in names] if set(value) == set(names) else None
            return value if isinstance(value, list) and len(value) == len(names) else None
        raw = ordered(context.get('raw_callee_arguments'))
        contextual = ordered(context.get('contextual_arguments'))
        if raw is None or contextual is None:
            continue
        context['native_input_roles'] = dict(
            review_sha256=digest,
            arguments=[dict(position=index, **role, raw_callee_expression=raw[index],
                            contextual_input=contextual[index]) for index,role in enumerate(arguments)],
            scope='Original VM push order. Conversion labels identify the native numeric pop operation, not a converted result. All native/VM numeric, storage, resource and object prerequisites remain obligations; symbolic captures retain their caller producer-time epoch.')
