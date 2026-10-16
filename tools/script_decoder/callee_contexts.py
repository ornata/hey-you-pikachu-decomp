"""Conditional literal-callsite inputs to the first unchanged formal consumer.

Rows retain shared expressions and boxed operand evidence. They establish neither
path feasibility nor native outcomes, and never globally substitute formals.
"""

from .context import ROOT
from .formats import integer_operand
from .local_flow import local_preservation_contracts, vm_comparison_contract
from .native_frames import native_frame_reviews
from .responses import annotate_literal_native_contexts, annotate_forwarded_native_contexts, contextual_program_ids


def context_snapshot(reviews=None, *, root=ROOT):
    """Validate one source snapshot for reuse during a single catalog analysis.

    Supplied reviews must be the freshly source-validated catalog frame reviews;
    snapshots are ephemeral and must not be retained across source mutations.
    """
    if reviews is None:
        reviews = native_frame_reviews(root)
    preservation = local_preservation_contracts(reviews, root)
    preservation['context_vm_contract'] = vm_comparison_contract(root)
    return reviews, preservation


def literal_native_argument_contexts(instructions, *, snapshot=None, root=ROOT, limit=256):
    """Return per-CALL conditional inputs using a bounded exclusive callee prefix."""
    edges = []
    for row in instructions:
        if row.get('name') != 'SCRIPT_OP_CALL':
            continue
        operands = row.get('operands', [])
        target = integer_operand(operands[0]) if len(operands) == 1 else None
        edges.append(dict(instruction_offset=row['offset'], target_offset=target))
    graph = dict(script_calls=edges)
    annotate_literal_native_contexts(instructions, [graph], root=root, limit=limit,
                                    validated_snapshot=snapshot)
    return [edge['conditional_native_argument_context'] for edge in edges
            if 'conditional_native_argument_context' in edge]



def contextual_timed_program_ids(contexts):
    """Collect only reviewed script_id argument references, retaining domain separation."""
    return contextual_program_ids([dict(script_calls=[
        dict(conditional_native_argument_context=context) for context in contexts])])


def forwarded_native_argument_contexts(instructions, *, configuration, snapshot=None,
                                      root=ROOT, limit=256):
    """Return exact caller-time direct-LOAD captures, without re-resolving variables."""
    edges = []
    for row in instructions:
        if row.get('name') != 'SCRIPT_OP_CALL':
            continue
        operands = row.get('operands', [])
        target = integer_operand(operands[0]) if len(operands) == 1 else None
        edges.append(dict(instruction_offset=row['offset'], target_offset=target))
    graph = dict(script_calls=edges)
    annotate_forwarded_native_contexts(instructions, [graph], configuration=configuration,
                                      root=root, limit=limit, validated_snapshot=snapshot)
    return [edge['conditional_forwarded_native_argument_context'] for edge in edges
            if 'conditional_forwarded_native_argument_context' in edge]
