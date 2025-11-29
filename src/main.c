#include <stdio.h>
#include "../include/cpu.h"
#include "../include/deviceinfo.h"

int main(void) {
    CpuInfo cpu;

    // read CPU
    if (get_cpu_info(&cpu) != 0) {
        fprintf(stderr, "Failed to read CPU info.\n");
        return 1;
    }

    // Device Info
    char serial[128]  = {0};
    char uuid[128]    = {0};
    char board[128]   = {0};
    char product[128] = {0};

    get_serial(serial, sizeof(serial));
    get_hardware_uuid(uuid, sizeof(uuid));
    get_board_id(board, sizeof(board));
    get_product_name(product, sizeof(product));

    // Ausgabe
    printf("=== CPU INFO TEST ===\n");
    printf("Model:         %s\n", cpu.model);
    printf("Architecture:  %s\n", cpu.architecture);
    printf("Cores (phys):  %d\n", cpu.physical_cores);
    printf("Cores (logic): %d\n", cpu.logical_cores);

    printf("\n=== DEVICE INFO ===\n");
    printf("Serial Number : %s\n", serial[0]  ? serial  : "<unavailable>");
    printf("Hardware UUID : %s\n", uuid[0]    ? uuid    : "<unavailable>");
    printf("Board ID      : %s\n", board[0]   ? board   : "<unavailable>");
    printf("Product Name  : %s\n", product[0] ? product : "<unavailable>");

    return 0;
}
