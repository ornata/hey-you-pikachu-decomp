/// \file
/// Names authored feedback and counting-question values carried by voice commands.

#ifndef VOICE_FEEDBACK_QUESTIONS_H
#define VOICE_FEEDBACK_QUESTIONS_H

/// Message category carrying the retained insulting vocabulary family.
enum { VOICE_INSULT_MESSAGE_TYPE = 4 };

/// Insult-label families, independent of a recipient's reaction or friendship change.
typedef enum VoiceInsultValue {
    VOICE_INSULT_ELECTRIC_RAT_OR_HATE = 0,
} VoiceInsultValue;

/// Message category carrying the retained praise vocabulary families.
enum { VOICE_PRAISE_MESSAGE_TYPE = 5 };

/// Praise-label families; values describe requests, not approval or action outcomes.
typedef enum VoicePraiseValue {
    VOICE_PRAISE_GREAT_JOB = 0,
    VOICE_PRAISE_CUTE = 1,
    VOICE_PRAISE_TERRIFIC = 2,
} VoicePraiseValue;

/// Message category carrying counting questions in the reviewed vocabulary.
enum { VOICE_COUNT_QUESTION_MESSAGE_TYPE = 11 };

/// Counting-question labels, independent of the count or validity of an answer.
typedef enum VoiceCountQuestionValue {
    VOICE_COUNT_QUESTION_HOW_MANY = 0,
} VoiceCountQuestionValue;

#endif
