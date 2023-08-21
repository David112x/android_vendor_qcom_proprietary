/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#include <tinyalsa/asoundlib.h>

__BEGIN_DECLS

#define SEC_TDM_RX_HOSTLESS 48
#define TERT_TDM_TX_HOSTLESS 49
#define QUAT_TDM_RX_HOSTLESS 50
#define QUAT_TDM_TX_HOSTLESS 51
#define QUIN_TDM_RX_HOSTLESS 52
#define QUIN_TDM_TX_HOSTLESS 53

#define MAX_SND_CARD 8

struct hostless_config {
    bool enable;
    struct pcm *pcm_tx;
    struct pcm *pcm_rx;
};

enum {
    MERC_SESSION = 0,
    A2B_SESSION,
    MAX_SESSION
};

enum {
    PCM_INPUT = 0,
    PCM_OUTPUT,
    PCM_MAX
};

enum {
    ROUTE_MIXER = 0,
    ROUTE_VALUE,
    ROUTE_MAX
};

struct snd_card_info {
    pthread_mutex_t lock;
    int snd_card;
    struct mixer *mixer;
    struct hostless_config hostless[MAX_SESSION];
};

typedef struct auto_audio {
    pthread_mutex_t lock;
    struct snd_card_info snd_card[MAX_SND_CARD];
} auto_audio_t;

int32_t auto_audio_ext_init(void);
void auto_audio_ext_deinit(void);
int32_t auto_audio_ext_set_snd_card(int snd_card);
int32_t auto_audio_ext_enable_hostless(int snd_card);
void auto_audio_ext_disable_hostless(int snd_card);
int32_t auto_audio_ext_enable_hostless_all(void);
void auto_audio_ext_disable_hostless_all(void);

void wakelock_init();
void wakelock_deinit();
void wakelock_acquire();
void wakelock_release();

__END_DECLS
