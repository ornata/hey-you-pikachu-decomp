"""Source-bound input roles for selected VM natives, in original push order.

Roles label boxed input positions. They establish neither converted values nor
resource validity, actor identity, message delivery or successful native effects.
"""

import hashlib
import json

# Frozen expanded reviews, renewed only after independent source/ROM review.
ROLE_BINDINGS = {'actorScriptCreateItem': {'arguments': [{'conversion': 'float', 'name': 'x'},
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
                             'review_sha256': '3aac60e3bdcdcb61153ac107a12540bff1511efd9bcdc1db6070a4e06b8c5d57'},
 'actorScriptGetAreaPathPointCoordinate': {'arguments': [{'conversion': 'integer',
                                                          'name': 'path_index'},
                                                         {'conversion': 'integer',
                                                          'name': 'point_index'},
                                                         {'conversion': 'integer', 'name': 'axis'}],
                                           'review_sha256': '6f49c229f48df9f0b3c587152d18f0c42167c1f9c0c9368a96b569b471c5ed15'},
 'actorScriptGetBoundingRadius': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                  'review_sha256': '904e0e80b802309de14fdac4c9d4cc31d3ec958ea5229c6fc238bbceca520dc3'},
 'actorScriptGetCreatureActionFlags': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                                       'review_sha256': '6badb329d4f795e34019698dde834f36a52d0c5c45ddff3270bbb14cb9ef6482'},
 'actorScriptGetItemCollectionValue': {'arguments': [{'conversion': 'integer',
                                                      'name': 'item_kind'}],
                                       'review_sha256': '129379fb30027127f5fa8b46e0444e11d0ef43de8e7529f57d69241ef4ba9040'},
 'actorScriptGetItemKind': {'arguments': [{'conversion': 'integer', 'name': 'handle'}],
                            'review_sha256': '3d9d842249982c2b8848170a8030dc70c208c513cf0fd8cd28bd6b9edac8f88a'},
 'actorScriptGetPokemonNameDictionaryGroup': {'arguments': [{'conversion': 'integer',
                                                             'name': 'catalog_index'}],
                                              'review_sha256': '650a60652297d3442a317e91db4a379094633b61de8eaae74e40847bd21ee36c'},
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
