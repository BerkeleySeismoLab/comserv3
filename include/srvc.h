#ifndef SRVC_H
#define SRVC_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#define CSCM_ATTACH 0           /* Attach me to server if not already attached */
#define CSCM_DATA_BLK 1         /* Request combinations of data and blockettes */
#define CSCM_LINK 2             /* link format info */
#define CSCM_CAL 3              /* Calibrator info */
#define CSCM_DIGI 4             /* Digitizer info */
#define CSCM_CHAN 5             /* Channel recording status */
#define CSCM_ULTRA 6            /* Misc flags and Comm Event names */
#define CSCM_LINKSTAT 7         /* Accumulated link performance info */
#define CSCM_CLIENTS 8          /* Get information about clients */
#define CSCM_UNBLOCK 9          /* Unblock packets associated with client N */
#define CSCM_RECONFIGURE 10     /* Link reconfigure request */
#define CSCM_SUSPEND 11         /* Suspend link operation */
#define CSCM_RESUME 12          /* Resume link operation */
#define CSCM_CMD_ACK 13         /* Client acknowledges command finished */
#define CSCM_TERMINATE 14       /* Terminate server */
#define CSCM_LINKSET 15         /* Set server link parameters */
#define CSCM_SHELL 20           /* Send shell command */
#define CSCM_VCO 21             /* Set VCO frequency */
#define CSCM_LINKADJ 22         /* Change link parameters */
#define CSCM_MASS_RECENTER 23   /* Mass recentering */
#define CSCM_CAL_START 24       /* Start calibration */
#define CSCM_CAL_ABORT 25       /* Abort calibration */
#define CSCM_DET_ENABLE 26      /* Detector on/off */
#define CSCM_DET_CHANGE 27      /* Change detector parameters */
#define CSCM_REC_ENABLE 28      /* Change recording status */
#define CSCM_COMM_EVENT 29      /* Set remote command mask */
#define CSCM_DET_REQUEST 30     /* Request detector parameters */
#define CSCM_DOWNLOAD 31        /* Start file download */
#define CSCM_DOWNLOAD_ABORT 32  /* Abort download */
#define CSCM_UPLOAD 33          /* Start file upload */
#define CSCM_UPLOAD_ABORT 34    /* Abort upload */
#define CSCM_FLOOD_CTRL 35      /* Turn flooding on and off */

#endif
