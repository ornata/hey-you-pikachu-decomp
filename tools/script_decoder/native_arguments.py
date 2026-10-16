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
                           'review_sha256': '9b18f75776f78f8ca9a4103e2180cc6781456d7e617b7bfaaa4063944bd114a1'},
 'actorScriptCreateStationaryItem': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                                   {'conversion': 'float', 'name': 'y'},
                                                   {'conversion': 'float', 'name': 'z'},
                                                   {'conversion': 'float', 'name': 'pitch'},
                                                   {'conversion': 'float', 'name': 'yaw'},
                                                   {'conversion': 'integer', 'name': 'kind'}],
                                     'review_sha256': 'db7731e647deefa87963ee64d418ad6ecc13af32f12aef2f4618df7238de4bee'},
 'actorScriptFadePrimaryOverlayBlend': {'arguments': [{'conversion': 'integer', 'name': 'red'},
                                                      {'conversion': 'integer', 'name': 'green'},
                                                      {'conversion': 'integer', 'name': 'blue'},
                                                      {'conversion': 'integer', 'name': 'alpha'},
                                                      {'conversion': 'float', 'name': 'duration'}],
                                        'review_sha256': '275fd5b5bb2ab14327604009b5619ddfc87c988e9582fdb67b40725cace913d2'},
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
                                     'review_sha256': '2e4c77f06f0afdd75c88fd3aeab4df01a9c27b4116060fd36bd7fcb94d221fcf'},
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
                                     'review_sha256': '3c8a3bad996d69f075f507c29acaf57d1153cbba59b5923229185de5820b4506'},
 'actorScriptFindNearestInFacingCone': {'arguments': [{'conversion': 'integer',
                                                       'name': 'minimum_kind'},
                                                      {'conversion': 'integer',
                                                       'name': 'maximum_kind'},
                                                      {'conversion': 'float',
                                                       'name': 'distance_limit'},
                                                      {'conversion': 'float',
                                                       'name': 'cone_angle_degrees'}],
                                        'review_sha256': 'a47c97dcaa3c97ed9f7dd4437eb9a3ed7e10294ed88cde45fdaaf5822639fa27'},
 'actorScriptFindObjectInFacingCone': {'arguments': [{'conversion': 'integer',
                                                      'name': 'minimum_kind'},
                                                     {'conversion': 'integer',
                                                      'name': 'maximum_kind'},
                                                     {'conversion': 'float',
                                                      'name': 'distance_limit'},
                                                     {'conversion': 'float',
                                                      'name': 'cone_angle_degrees'}],
                                       'review_sha256': '269036d997d9dec712b330abd9dabd1e519c6a683b230420fd8c546216b652e4'},
 'actorScriptGetActorKind': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                             'review_sha256': 'a323fc4923fabc3bdd512d8c163d9246dc964e826101538c1a1abc5951b56dab'},
 'actorScriptGetAreaPathPointCoordinate': {'arguments': [{'conversion': 'integer',
                                                          'name': 'path_index'},
                                                         {'conversion': 'integer',
                                                          'name': 'point_index'},
                                                         {'conversion': 'integer', 'name': 'axis'}],
                                           'review_sha256': '86b5998903dea69b9c71670ac35eef019d4ce209d0e670e6984b5e644644cad5'},
 'actorScriptGetBoundingRadius': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                  'review_sha256': '5211bba38fa632d926e6a47254ff87ffd96a71983ea4da16bed645982544d114'},
 'actorScriptGetCreatureActionFlags': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                       'review_sha256': 'f4ad97f9242622811f68142e40dfae65e95f0ad79020001386b598136572a172'},
 'actorScriptGetItemCollectionValue': {'arguments': [{'conversion': 'integer',
                                                      'name': 'item_kind'}],
                                       'review_sha256': '495861521475b0d20768ffa631decca8b28c51ce12a515d8197ac5c757173dbf'},
 'actorScriptGetItemKind': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                            'review_sha256': '0a3aba3e1d052d4646c7cfe33d7c6b4f37fa012f076275644b4db0546857499b'},
 'actorScriptGetItemState': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                             'review_sha256': '2bc7d7c13ef7685484a94ed8e3ad5f4800afacc4dfb5321195caacaee4f107d6'},
 'actorScriptGetObjectType': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                              'review_sha256': '806a2a64369bad2651f1b80ea29852f8baae0372682c2975d8bee52aca02de80'},
 'actorScriptGetPokemonNameDictionaryGroup': {'arguments': [{'conversion': 'integer',
                                                             'name': 'catalog_index'}],
                                              'review_sha256': 'ed70c8056b89861002cf6f8a7ad2923de93ee18ad5c2c4b065dae76fbcb738c7'},
 'actorScriptGetProgressValue': {'arguments': [{'conversion': 'integer', 'name': 'field_id'}],
                                 'review_sha256': '8a9f3b4f5d82dd265972330928136d5da8ac1e6edfebed3a4748f858bc182813'},
 'actorScriptPassThroughInteger': {'arguments': [{'conversion': 'integer', 'name': 'value'}],
                                   'review_sha256': '7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8'},
 'actorScriptPostMessage': {'arguments': [{'conversion': 'integer', 'name': 'recipient_handle'},
                                          {'conversion': 'integer', 'name': 'message_type'},
                                          {'conversion': 'integer', 'name': 'argument0'},
                                          {'conversion': 'integer', 'name': 'argument2'},
                                          {'conversion': 'integer', 'name': 'argument3'},
                                          {'conversion': 'integer', 'name': 'argument4'}],
                            'review_sha256': '28d965324d5acfa44eab20003067e73397c8efccc0dffcb61a97cfb0b16b4c2e'},
 'actorScriptCreateScrollBoard': {'arguments': [{'conversion': 'integer', 'name': 'preset'},
                                               {'conversion': 'integer', 'name': 'style'},
                                               {'conversion': 'integer', 'name': 'columns'},
                                               {'conversion': 'integer', 'name': 'lines'},
                                               {'conversion': 'float', 'name': 'x'},
                                               {'conversion': 'float', 'name': 'y'},
                                               {'conversion': 'float', 'name': 'z'},
                                               {'conversion': 'float', 'name': 'rotation_x'},
                                               {'conversion': 'float', 'name': 'rotation_y'},
                                               {'conversion': 'float', 'name': 'rotation_z'},
                                               {'conversion': 'integer', 'name': 'red'},
                                               {'conversion': 'integer', 'name': 'green'},
                                               {'conversion': 'integer', 'name': 'blue'},
                                               {'conversion': 'integer', 'name': 'alpha'},
                                               {'conversion': 'integer', 'name': 'leading_outline'},
                                               {'conversion': 'integer', 'name': 'trailing_outline'},
                                               {'conversion': 'float', 'name': 'scale'},
                                               {'conversion': 'float', 'name': 'speed'}],
                                 'review_sha256': '1a38c22efae0945458c734de71fa68f8972cf03b5c5d3ced1dfe94f543d8234b'},
 'actorScriptSetDoorAccessState': {'arguments': [{'conversion': 'integer', 'name': 'created_door_index'},
                                                {'conversion': 'integer', 'name': 'access_state'}],
                                  'review_sha256': 'ec82906b443a5a0ab37c9ed4840281c01a10cbea78dec04753eccb7fbe8b71d9'},
 'actorScriptSetDestination': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                             {'conversion': 'float', 'name': 'y'},
                                             {'conversion': 'float', 'name': 'z'},
                                             {'conversion': 'float', 'name': 'distance_allowance'}],
                               'review_sha256': '2eb140a4be88760bb2660b82ea3265a66b9abb9a003402ce4147cffa1f5e71cd'},
 'actorScriptSetProgressValue': {'arguments': [{'conversion': 'integer', 'name': 'field_id'},
                                               {'conversion': 'integer',
                                                'name': 'requested_value'}],
                                 'review_sha256': '0139883166d83c2855045ef88d951316e7006dc43eea5168cb8dd228b7d2d6ff'},
 'actorScriptTrackObject': {'arguments': [{'conversion': 'integer', 'name': 'target_handle'},
                                          {'conversion': 'float', 'name': 'distance_allowance'}],
                            'review_sha256': '1bfbbac0f79827263cb0844250e840a2e02613e530d00a8ff88a87aa926b62db'}}


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
