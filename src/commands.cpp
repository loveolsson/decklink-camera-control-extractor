#ifdef ARDUINO_ARCH_ESP32
#include "include/commands.h"

#include "include/defines.h"  // for NUM, CommandInfo
#else
#include "commands.h"
#include "defines.h"  // for NUM, CommandInfo
#endif

#include <stdint.h>  // for uint8_t

struct CommandGroup {
    const Command *commands;
    const int commandCount;
};

static const Command lensCommands[] = {
    {"Focus"},
    {"Instantaneous autofocus"},
    {"Aperture (f-stop)"},
    {"Aperture (normalised)"},
    {"Aperture (ordinal)"},
    {"Instantaneous auto aperture"},
    {"Optical image stabilisation"},
    {"Set absolute zoom (mm)"},
    {"Set absolute zoom (normalised)"},
    {"Set continuous zoom (speed)"},
};

static const CommandGroup lensCommandGroup = {lensCommands, NUM(lensCommands)};

static const Command videoCommands[] = {
    {"Video mode"},
    {"Gain (up to Camera 4.9)"},
    {"Manual White Balance"},
    {"Set auto WB"},
    {"Restore auto WB"},
    {"Exposure (us)"},
    {"Exposure (ordinal)"},
    {"Dynamic Range Mode"},
    {"Video sharpening level"},
    {"Recording format"},
    {"Set auto exposure mode"},
    {"Shutter angle"},
    {"Shutter speed"},
    {"Gain"},
    {"ISO"},
    {"Display LUT"},
};

static const CommandGroup videoCommandGroup = {videoCommands, NUM(videoCommands)};

static const Command audioCommands[] = {
    {"Mic level"},  {"Headphone level"}, {"Headphone program mix"}, {"Speaker level"},
    {"Input type"}, {"Input levels"},    {"Phantom power"},
};

static const CommandGroup audioCommandGroup = {audioCommands, NUM(audioCommands)};

static const Command outputCommands[] = {
    {"Overlay enables"},
    {"Frame guides style (Camera 3.x)"},
    {"Frame guides opacity (Camera 3.x)"},
    {"Overlays (replaces .1 and .2 above from Cameras 4.0)"},
};

static const CommandGroup outputCommandGroup = {outputCommands, NUM(outputCommands)};

static const Command displayCommands[] = {
    {"Brightness"},
    {"Exposure and focus tools"},
    {"Zebra level"},
    {"Peaking level"},
    {"Color bar enable"},
    {"Focus Assist"},
    {"Program return feed enable"},
};

static const CommandGroup displayCommandGroup = {displayCommands, NUM(displayCommands)};

static const Command tallyCommands[] = {
    {"Tally brightness"},
    {"Front tally brightness"},
    {"Rear tally brightness"},
};

static const CommandGroup tallyCommandGroup = {tallyCommands, NUM(tallyCommands)};

static const Command referenceCommands[] = {
    {"Source"},
    {"Offset"},
};

static const CommandGroup referenceCommandGroup = {referenceCommands, NUM(referenceCommands)};

static const Command configurationCommands[] = {
    {"Real Time Clock"},
    {"System language"},
    {"Timezone"},
    {"Location"},
};

static const CommandGroup configurationCommandGroup = {configurationCommands,
                                                       NUM(configurationCommands)};

static const Command colorCommands[] = {
    {"Lift Adjust"},     {"Gamma Adjust"}, {"Gain Adjust"},  {"Offset Adjust"},
    {"Contrast Adjust"}, {"Luma mix"},     {"Color Adjust"}, {"Correction Reset Default"},
};

static const CommandGroup colorCommandGroup = {colorCommands, NUM(colorCommands)};

static const Command mediaCommands[] = {
    {"Codec"},
    {"Transport mode"},
    {"Playback Control"},
};

static const CommandGroup mediaCommandGroup = {mediaCommands, NUM(mediaCommands)};

static const Command ptzCommands[] = {
    {"Pan/Tilt Velocity"},
    {"Memory Preset"},
};

static const CommandGroup ptzCommandGroup = {ptzCommands, NUM(ptzCommands)};

static const CommandGroup *groups[] = {
    &lensCommandGroup,      &videoCommandGroup,
    &audioCommandGroup,     &outputCommandGroup,
    &displayCommandGroup,   &tallyCommandGroup,
    &referenceCommandGroup, &configurationCommandGroup,
    &colorCommandGroup,     nullptr,
    &mediaCommandGroup,     &ptzCommandGroup,
};

const Command *
GetCommandFromData(const CommandInfo *data)
{
    const uint8_t category = data->category;
    const uint8_t param    = data->parameter;

    if (category >= NUM(groups)) {
        return nullptr;
    }

    const CommandGroup *group = groups[category];

    if (group == nullptr) {
        return nullptr;
    }

    if (group->commandCount <= param) {
        return nullptr;
    }

    return &group->commands[param];
}
