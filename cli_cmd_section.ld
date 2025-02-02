/* Linker script */
SECTIONS {
    .cli_cmd_section : {
        __start_cli_cmd_section = .;
        KEEP(*(.cli_cmd_section))
        __stop_cli_cmd_section = .;
    } > drom0_0_seg
} 
