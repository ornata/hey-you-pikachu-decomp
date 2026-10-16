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
                           'review_sha256': 'be5fec35452bd2c797d1d54c2e7db1f55ea56a8f46cacf1c0a9a166bd553067e'},
 'actorScriptCreateStationaryItem': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                                   {'conversion': 'float', 'name': 'y'},
                                                   {'conversion': 'float', 'name': 'z'},
                                                   {'conversion': 'float', 'name': 'pitch'},
                                                   {'conversion': 'float', 'name': 'yaw'},
                                                   {'conversion': 'integer', 'name': 'kind'}],
                                     'review_sha256': '3b81d5ee39982964f45c8975d97207a75f93eb044357e1130439d01504ec057e'},
 'actorScriptFadePrimaryOverlayBlend': {'arguments': [{'conversion': 'integer', 'name': 'red'},
                                                      {'conversion': 'integer', 'name': 'green'},
                                                      {'conversion': 'integer', 'name': 'blue'},
                                                      {'conversion': 'integer', 'name': 'alpha'},
                                                      {'conversion': 'float', 'name': 'duration'}],
                                        'review_sha256': 'aa465ae778bb90af7d7b8867a10990edc130e2ade0cddb3d5ce62265484ae6b8'},
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
                                     'review_sha256': 'ddc11104aeb890df1052b9de3a94972a3c4335fbf8c57e84bb12d6f5260c0a96'},
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
                                     'review_sha256': '54b675af968fd60244693eb45d2726205d1010764cecf9eeb04152b800b07b09'},
 'actorScriptFindNearestInFacingCone': {'arguments': [{'conversion': 'integer',
                                                       'name': 'minimum_kind'},
                                                      {'conversion': 'integer',
                                                       'name': 'maximum_kind'},
                                                      {'conversion': 'float',
                                                       'name': 'distance_limit'},
                                                      {'conversion': 'float',
                                                       'name': 'cone_angle_degrees'}],
                                        'review_sha256': 'c020150ce50f96a3b25ffe2b19d38b1785938db2313f687f9d63d0d522520327'},
 'actorScriptFindObjectInFacingCone': {'arguments': [{'conversion': 'integer',
                                                      'name': 'minimum_kind'},
                                                     {'conversion': 'integer',
                                                      'name': 'maximum_kind'},
                                                     {'conversion': 'float',
                                                      'name': 'distance_limit'},
                                                     {'conversion': 'float',
                                                      'name': 'cone_angle_degrees'}],
                                       'review_sha256': '302d24fae5d2e9f8ca5dcc8ef4893336810b12b9948ede79dfa394dcf449a236'},
 'actorScriptGetActorKind': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                             'review_sha256': 'ee5d8dd13bb391df6e0cb37c4c2d85a0262aeaf9f6b231ad891be1ae60dfbddc'},
 'actorScriptGetAreaPathPointCoordinate': {'arguments': [{'conversion': 'integer',
                                                          'name': 'path_index'},
                                                         {'conversion': 'integer',
                                                          'name': 'point_index'},
                                                         {'conversion': 'integer', 'name': 'axis'}],
                                           'review_sha256': '86b5998903dea69b9c71670ac35eef019d4ce209d0e670e6984b5e644644cad5'},
 'actorScriptGetBoundingRadius': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                  'review_sha256': 'dc9f6488197be3f13396ca66aa0d1f05c3b736b5b559844a9397be72408a883c'},
 'actorScriptGetCreatureActionFlags': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                       'review_sha256': '6badb329d4f795e34019698dde834f36a52d0c5c45ddff3270bbb14cb9ef6482'},
 'actorScriptGetItemCollectionValue': {'arguments': [{'conversion': 'integer',
                                                      'name': 'item_kind'}],
                                       'review_sha256': '9f2af6cbbe44cdf53c1669b7d7b7bd8ea8743f55d1377a944015a5ce94dd80b5'},
 'actorScriptGetItemKind': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                            'review_sha256': '56d20013f5ff9c2683abc19d163c934d290ac4c9b197648ef7240c1c3ff5bcbc'},
 'actorScriptGetItemState': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                             'review_sha256': '7a6a8743a060b8ed55be2ae35362eaa184279d180783c9dbbffd52d3c024e453'},
 'actorScriptGetObjectType': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                              'review_sha256': '978e95b72d62f43ff396f93adb5f9a939d115351be5907b7d5302f4f4b56ddb8'},
 'actorScriptGetPokemonNameDictionaryGroup': {'arguments': [{'conversion': 'integer',
                                                             'name': 'catalog_index'}],
                                              'review_sha256': '0f116e4f26833514127c51f75a4696c3106f50a7d3fc733436c2051bcf568199'},
 'actorScriptGetProgressValue': {'arguments': [{'conversion': 'integer', 'name': 'field_id'}],
                                 'review_sha256': 'e6eadc8d3c9e790d1e6c550a6894d9f230c4e6ca389547971f621fb779136383'},
 'actorScriptPassThroughInteger': {'arguments': [{'conversion': 'integer', 'name': 'value'}],
                                   'review_sha256': '7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8'},
 'actorScriptPostMessage': {'arguments': [{'conversion': 'integer', 'name': 'recipient_handle'},
                                          {'conversion': 'integer', 'name': 'message_type'},
                                          {'conversion': 'integer', 'name': 'argument0'},
                                          {'conversion': 'integer', 'name': 'argument2'},
                                          {'conversion': 'integer', 'name': 'argument3'},
                                          {'conversion': 'integer', 'name': 'argument4'}],
                            'review_sha256': '9dac2d5d4c12f0a7f0d0df8721448b4d5cdce77915e6a57ec32966e13333d77f'},
 'actorScriptSetDestination': {'arguments': [{'conversion': 'float', 'name': 'x'},
                                             {'conversion': 'float', 'name': 'y'},
                                             {'conversion': 'float', 'name': 'z'},
                                             {'conversion': 'float', 'name': 'distance_allowance'}],
                               'review_sha256': '1297ecd8614b39effce518ac0eb627efabc9b42fed91784735e5fe3835b92d8c'},
 'actorScriptSetProgressValue': {'arguments': [{'conversion': 'integer', 'name': 'field_id'},
                                               {'conversion': 'integer',
                                                'name': 'requested_value'}],
                                 'review_sha256': '0139883166d83c2855045ef88d951316e7006dc43eea5168cb8dd228b7d2d6ff'},
 'actorScriptTrackObject': {'arguments': [{'conversion': 'integer', 'name': 'target_handle'},
                                          {'conversion': 'float', 'name': 'distance_allowance'}],
                            'review_sha256': '19a800aa635ba4055d743c23261c202bc2c51ea2f359ac39cfe62916fed5cc91'}}


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
