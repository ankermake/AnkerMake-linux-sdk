#ifndef __CANNA_ICDC_H__
#define __CANNA_ICDC_H__

enum AMIXER_CONTROLS {
	Master_Playback_Volume       = 1,
	Mic_Volume                   = 6,
	ADC_High_Pass_Filter_Switch  = 7,
	ADC_Mux                      = 16,
	Playback_Mixer_Volume        = 3,
	Alerts_Playback_Volume       = 18,
	Content_Playback_Volume      = 19,
	Dialog_Playback_Volume       = 17,
	Digital_Capture_Mixer_Volume = 5,
	Digital_Capture_Volume       = 4,
	Digital_Playback_mute        = 8,
	Mix0                         = 9,
	Mix1                         = 10,
	Mix2                         = 11,
	Mix3                         = 12,
	Mux0                         = 13,
	Mux2                         = 14,
	TITANIUM_Playback_Volume     = 2,
	VDIGITAL_BYPASS_Switch       = 15,
};

enum AMIXER_TYPE {
	BOOLEAN,
	ENUMERATED,
	INTEGER,
	END,
};

enum AMIXER_VALUE {
	NORMAL_INPUTS = 0,
	CROSS_INPUTS,
	MIXED_INPUTS,
	ZERO_INPUTS,
	PLAYBACK_DAC_ONLY = 0,
	PLAYBACK_DAC_ADC,
	RECORD_INPUT_ONLY = 0,
	RECORD_INPUT_DAC,
};

struct mixer_controls {
	unsigned int mixer_type;
	unsigned int mixer_id;
	unsigned int mixer_value;
};

struct mixer_mode {
	char name[15];
	struct mixer_controls *controls;
};





static struct mixer_controls route_linein[] = {
	[0] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix0,
		.mixer_value = MIXED_INPUTS,
	},
	[1] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix1,
		.mixer_value = NORMAL_INPUTS,
	},
	[2] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix2,
		.mixer_value = NORMAL_INPUTS,
	},
	[3] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix3,
		.mixer_value = NORMAL_INPUTS,
	},
	[4] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mux0,
		.mixer_value = PLAYBACK_DAC_ADC,
	},
	[5] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mux2,
		.mixer_value = RECORD_INPUT_DAC,
	},
	[6] = {
		.mixer_type = BOOLEAN,
		.mixer_id = VDIGITAL_BYPASS_Switch,
		.mixer_value = 1,
	},
	[7] = {
		.mixer_type = END,
	},
};

static struct mixer_controls route_normal[] = {
	[0] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix0,
		.mixer_value = NORMAL_INPUTS,
	},
	[1] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix1,
		.mixer_value = NORMAL_INPUTS,
	},
	[2] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix2,
		.mixer_value = NORMAL_INPUTS,
	},
	[3] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mix3,
		.mixer_value = ZERO_INPUTS,
	},
	[4] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mux0,
		.mixer_value = PLAYBACK_DAC_ONLY,
	},
	[5] = {
		.mixer_type = ENUMERATED,
		.mixer_id = Mux2,
		.mixer_value = RECORD_INPUT_ONLY,
	},
	[6] = {
		.mixer_type = BOOLEAN,
		.mixer_id = VDIGITAL_BYPASS_Switch,
		.mixer_value = 0,
	},
	[7] = {
		.mixer_type = END,
	},
};

static struct mixer_mode mixer_modes[] = {
	[0] = {
		.name = "linein",
		.controls = route_linein,
	},
	[1] = {
		.name = "normal",
		.controls = route_normal,
	},
};

#endif /* __CANNA_ICDC_H__ */
