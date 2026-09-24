/*
 * config.c - CLI parsing for winlock
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "main.h"

static void usage(void) {
    fprintf(stderr,
        "usage: winlock <window_id> [options...]\n"
        "\n"
        "X11 window control utility. Locks window position by default.\n"
        "Stays alive to maintain locks and input grabs.\n"
        "\n"
        "arguments:\n"
        "  window_id              X11 window ID (hex 0x... or decimal)\n"
        "\n"
        "options:\n"
        "  --resize-lock [WxH]    lock window size (current size if WxH omitted)\n"
        "  --position-lock=0      disable default position locking\n"
        "  --no-right-click       block right-click (button 3)\n"
        "  --no-middle-click      block middle-click (button 2)\n"
        "  --skip-taskbar         hide from taskbar\n"
        "  --skip-pager           hide from pager/alt-tab\n"
        "  --no-decorations       remove title bar and borders\n"
        "  --opacity <0.0-1.0>    set window transparency\n"
        "  --oneshot              apply settings and exit (no event loop)\n"
        "  -h, --help             show this help message\n"
        "  -v, --version          show version number\n"
    );
}

static void version(void) {
    printf("winlock %s\n", VERSION);
}

void config_init(WinlockConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->position_lock = 1;
}

int config_parse(WinlockConfig *cfg, int argc, char **argv) {
    enum {
        OPT_RESIZE_LOCK = 256,
        OPT_POSITION_LOCK,
        OPT_NO_RIGHT_CLICK,
        OPT_NO_MIDDLE_CLICK,
        OPT_SKIP_TASKBAR,
        OPT_SKIP_PAGER,
        OPT_NO_DECORATIONS,
        OPT_OPACITY,
        OPT_ONESHOT,
    };

    static struct option long_opts[] = {
        {"resize-lock",     optional_argument, NULL, OPT_RESIZE_LOCK},
        {"position-lock",   required_argument, NULL, OPT_POSITION_LOCK},
        {"no-right-click",  no_argument,       NULL, OPT_NO_RIGHT_CLICK},
        {"no-middle-click", no_argument,       NULL, OPT_NO_MIDDLE_CLICK},
        {"skip-taskbar",    no_argument,       NULL, OPT_SKIP_TASKBAR},
        {"skip-pager",      no_argument,       NULL, OPT_SKIP_PAGER},
        {"no-decorations",  no_argument,       NULL, OPT_NO_DECORATIONS},
        {"opacity",         required_argument, NULL, OPT_OPACITY},
        {"oneshot",         no_argument,       NULL, OPT_ONESHOT},
        {"help",            no_argument,       NULL, 'h'},
        {"version",         no_argument,       NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hv", long_opts, NULL)) != -1) {
        switch (opt) {
        case OPT_RESIZE_LOCK:
            cfg->resize_lock = 1;
            if (optarg) {
                if (sscanf(optarg, "%dx%d", &cfg->width, &cfg->height) != 2) {
                    fprintf(stderr, "winlock: invalid size format '%s' (expected WxH)\n", optarg);
                    return -1;
                }
            }
            break;
        case OPT_POSITION_LOCK:
            cfg->position_lock = atoi(optarg);
            break;
        case OPT_NO_RIGHT_CLICK:
            cfg->no_right_click = 1;
            break;
        case OPT_NO_MIDDLE_CLICK:
            cfg->no_middle_click = 1;
            break;
        case OPT_SKIP_TASKBAR:
            cfg->skip_taskbar = 1;
            break;
        case OPT_SKIP_PAGER:
            cfg->skip_pager = 1;
            break;
        case OPT_NO_DECORATIONS:
            cfg->no_decorations = 1;
            break;
        case OPT_OPACITY:
            cfg->opacity = atof(optarg);
            if (cfg->opacity < 0.0 || cfg->opacity > 1.0) {
                fprintf(stderr, "winlock: opacity must be between 0.0 and 1.0\n");
                return -1;
            }
            break;
        case OPT_ONESHOT:
            cfg->oneshot = 1;
            break;
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
        case 'v':
            version();
            exit(EXIT_SUCCESS);
        default:
            usage();
            return -1;
        }
    }

    /* Window ID: required first positional arg */
    if (optind >= argc) {
        fprintf(stderr, "winlock: missing window_id argument\n");
        usage();
        return -1;
    }

    cfg->wid = (Window)strtoul(argv[optind], NULL, 0);
    if (!cfg->wid) {
        fprintf(stderr, "winlock: invalid window_id '%s'\n", argv[optind]);
        return -1;
    }

    return 0;
}
