#include <errno.h>
#include "vm.h"
#include "compiler.h"
#include "program.h"

vm_t vm;

static void execute(const char *source)
{
    program_t program = compile(source);
    vm_interpret(&vm, &program);
}

static int repl()
{
    char line[UINT8_MAX * 4];

    printf("> ");
    while (fgets(line, sizeof(line), stdin))
    {
        execute(line);
        printf("> ");
    }

    printf("\n");
    return 0;
}

static int read_file(const char *filename, char **output)
{
    int ret = 0;
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "ERROR: Couldn't open file %s, file not found\n", filename);
        ret = 1;
        goto exit;
    }

    if ((ret = fseek(file, 0L, SEEK_END)) < 0)
    {
        fprintf(stderr, "ERROR: Couldn't read file %s, %s\n", filename, strerror(ret));
        goto close;
    }

    size_t file_size = ftell(file);
    if (file_size < 0)
    {
        fprintf(stderr, "ERROR: Couldn't read file %s, %s\n", filename, strerror(file_size));
        goto close;
    }

    rewind(file);

    *output = (char *)malloc(file_size + 1);
    assert(*output != NULL);

    fread(*output, sizeof(char), file_size, file);
    (*output)[file_size] = '\0';

close:
    fclose(file);
exit:
    return ret;
}

static int from_file(const char *filename)
{
    char *content;
    int ret = read_file(filename, &content);

    if (ret == 0)
    {
        execute(content);
        free(content);
    }

    return ret;
}

int main(int argc, const char *argv[])
{
    int ret = 0;

    if (argc > 2)
    {
        fprintf(stderr, "Usage\n");
        return 64;
    }

    vm_init(&vm);

    if (argc == 1)
    {
        ret = repl();
    }
    else
    {
        ret = from_file(argv[1]);
    }

    vm_free(&vm);
    return ret;
}