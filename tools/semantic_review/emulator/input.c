/** Deterministic controller/VRU device-boundary inputs for semantic experiments. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define M64P_PLUGIN_PROTOTYPES
#include "m64p_plugin.h"
static uint32_t polls[65536], poll_count, poll_index;
static uint16_t results[1024][15];
static uint32_t result_count, result_index;
static int vru_present;
static unsigned speech_polls, speech_remaining;
static unsigned controller_override, controller_value;
static FILE *events;
static void event(const char *kind, unsigned a, unsigned b) {
    if (events) {
        fprintf(events, "{\"event\":\"%s\",\"a\":%u,\"b\":%u}\n", kind, a, b);
        fflush(events);
    }
}
EXPORT int ConfigureInput(const uint32_t *p, unsigned n, const uint16_t *r, unsigned nr, int present, const char *log) {
    if (n > 65536 || nr > 1024)
        return 1;
    if (events)
        fclose(events);
    events = fopen(log, "w");
    if (!events)
        return 2;
    if (n)
        memcpy(polls, p, n * sizeof(*p));
    if (nr)
        memcpy(results, r, nr * 15 * sizeof(*r));
    poll_count = n;
    poll_index = 0;
    result_count = nr;
    result_index = 0;
    vru_present = present;
    speech_polls = 0;
    speech_remaining = 0;
    controller_override = 0;
    controller_value = 0;
    return 0;
}
EXPORT void SetControllerValue(unsigned value) {
    controller_override = 1;
    controller_value = value;
}
EXPORT void ConfigureVRUTalking(unsigned polls) {
    speech_polls = polls;
}
EXPORT m64p_error PluginStartup(m64p_dynlib_handle core, void *context, void (*debug)(void *, int, const char *)) {
    return M64ERR_SUCCESS;
}
EXPORT m64p_error PluginShutdown(void) {
    if (events)
        fclose(events);
    events = NULL;
    return M64ERR_SUCCESS;
}
EXPORT m64p_error PluginGetVersion(m64p_plugin_type *type, int *version, int *api, const char **name,
                                   int *capabilities) {
    if (type)
        *type = M64PLUGIN_INPUT;
    if (version)
        *version = 0x10000;
    if (api)
        *api = 0x20101;
    if (name)
        *name = "Semantic deterministic input";
    if (capabilities)
        *capabilities = 0;
    return M64ERR_SUCCESS;
}
EXPORT void InitiateControllers(CONTROL_INFO info) {
    for (int i = 0; i < 4; i++) {
        info.Controls[i].Present = 0;
        info.Controls[i].RawData = 0;
        info.Controls[i].Plugin = 1;
        info.Controls[i].Type = CONT_TYPE_STANDARD;
    }
    info.Controls[0].Present = 1;
    info.Controls[3].Present = vru_present;
    info.Controls[3].Type = CONT_TYPE_VRU;
    event("controllers", 1, vru_present);
}
EXPORT void GetKeys(int control, BUTTONS *keys) {
    keys->Value = 0;
    if (control == 3 && speech_remaining) {
        keys->Value = 0x20;
        speech_remaining--;
        if (!speech_remaining)
            event("speech_finished", speech_polls, 0);
    }
    if (control == 0) {
        if (poll_count)
            keys->Value = polls[poll_index < poll_count ? poll_index : poll_count - 1];
        if (controller_override)
            keys->Value = controller_value;
        event("controller_poll", poll_index, keys->Value);
        poll_index++;
    }
}
EXPORT int RomOpen(void) {
    return 1;
}
EXPORT void RomClosed(void) {
    event("rom_closed", poll_index, result_index);
}
EXPORT void ControllerCommand(int control, unsigned char *command) {
}
EXPORT void ReadController(int control, unsigned char *command) {
}
EXPORT void SDL_KeyDown(int modifiers, int key) {
}
EXPORT void SDL_KeyUp(int modifiers, int key) {
}
EXPORT void RenderCallback(void) {
}
EXPORT void SendVRUWord(uint16_t length, uint16_t *word, uint8_t language) {
    event("dictionary_word", length, language);
    if (events) {
        fputs("{\"event\":\"dictionary_bytes\",\"hex\":\"", events);
        for (unsigned i = 0; i < length; i++)
            fprintf(events, "%04x", word[i]);
        fputs("\"}\n", events);
        fflush(events);
    }
}
EXPORT void SetMicState(int state) {
    event("microphone", state, 0);
    speech_remaining = state && result_index < result_count ? speech_polls : 0;
    if (state && speech_remaining)
        event("speech_started", speech_polls, 0);
}
static void result16(uint16_t *p, uint16_t value) {
    memcpy(p, &value, sizeof(value));
}
EXPORT void ReadVRUResults(uint16_t *error, uint16_t *count, uint16_t *mic, uint16_t *voice, uint16_t *length,
                           uint16_t *matches) {
    uint16_t empty[15] = {0};
    const uint16_t *r = result_index < result_count ? results[result_index] : empty;
    /* Public plugin results are host uint16 values. memcpy permits unaligned buffers. */
    result16(error, r[0]);
    result16(count, r[1]);
    result16(mic, r[2]);
    result16(voice, r[3]);
    result16(length, r[4]);
    for (unsigned i = 0; i < 10; i++)
        result16(matches + i, r[i + 5]);
    event("recognition_result", result_index, r[1]);
    result_index++;
}
EXPORT void ClearVRUWords(uint8_t length) {
    event("dictionary_clear", length, 0);
}
EXPORT void SetVRUWordMask(uint8_t length, uint8_t *mask) {
    event("dictionary_mask", length, 0);
    if (events) {
        fputs("{\"event\":\"mask_bytes\",\"hex\":\"", events);
        for (unsigned i = 0; i < length; i++)
            fprintf(events, "%02x", mask[i]);
        fputs("\"}\n", events);
        fflush(events);
    }
}
