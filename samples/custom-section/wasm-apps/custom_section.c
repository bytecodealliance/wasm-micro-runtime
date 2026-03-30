__attribute__((import_module("env"),
               import_name("get_custom_section_handle"))) int
get_custom_section_handle(const char *section_name);

__attribute__((import_module("env"), import_name("print_custom_section"))) void
print_custom_section(int handle);

__attribute__((export_name("run_demo"))) int
run_demo(void)
{
    static const char section_name[] = "demo";
    int handle = get_custom_section_handle(section_name);

    if (handle >= 0) {
        print_custom_section(handle);
    }

    return handle;
}
