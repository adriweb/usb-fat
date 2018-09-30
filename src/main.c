#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <keypadc.h>
#include "thinfat32.h"

unsigned char msd_Init(void);
void usb_Cleanup(void);
void msd_KeepAlive(void);

void key_Scan(void);
unsigned char key_Any(void);

void os_line(const char *str);
void wait_user(void);

void main(void) {
    char buffer[100];
    const char *file = "apples.txt";

    os_ClrHome();

    os_line("insert msd...");

    if (msd_Init()) {
        TFFile *fp;
        int rc;

        os_line("mounted msd");

        if (tf_init() == 0) {

            os_line("detected FAT32");
            os_PutStrLine("opening file \"");
            os_PutStrLine(file);
            os_PutStrLine("\"");
            os_NewLine();
            os_NewLine();

            if ((fp = tf_fopen(file, "r")) != NULL) {
		unsigned int size = fp->size;
		sprintf(buffer, "file size: %u bytes", size);
		os_line(buffer);
                tf_fread((uint8_t*)&buffer, size - 1, fp);
                buffer[size - 1] = '\0';
                os_line("file contents:");
                os_line(buffer);
                tf_fclose(fp);
            } else {
                os_line("did not open file.");
            }
        }

        usb_Cleanup();
    }

    /* flush */
    while(!os_GetCSC());
}

/* cannot use getcsc in usb */
void wait_user(void) {
    while(!key_Any()) {
        msd_KeepAlive();
    }
}

void os_line(const char *str) {
    os_PutStrLine(str);
    os_NewLine();
}
