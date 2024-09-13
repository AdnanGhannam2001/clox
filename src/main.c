#include <errno.h>
#include "common.h"
#include "vm.h"
#include "compiler.h"
#include "program.h"

// FIXME
vm_t vm;

static interpret_result_t execute(const char *source)
{
    program_t program;
    program_init(&program);

    compiler_t compiler;
    if (compiler_run(&compiler, source, &program) != 0)
    {
        program_free(&program);
        return INTERPRET_RESULT_COMPILE_ERROR;
    }

    vm_interpret(&vm, &program);
    program_free(&program);

    return INTERPRET_RESULT_OK;
}

static void repl()
{
    char line[UINT8_MAX * 4];

    printf("> ");
    while (fgets(line, sizeof(line), stdin))
    {
        execute(line);
        printf("> ");
    }

    printf("\n");
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

    long file_size = ftell(file);
    if (file_size < 0)
    {
        fprintf(stderr, "ERROR: Couldn't read file %s, %s\n", filename, strerror((int)file_size));
        goto close;
    }

    rewind(file);

    *output = (char *)malloc((size_t)file_size + 1);
    assert(*output != NULL);

    fread(*output, sizeof(char), (size_t)file_size, file);
    (*output)[file_size] = '\0';

close:
    fclose(file);
exit:
    return ret;
}

static interpret_result_t from_file(const char *content)
{
    return execute(content);
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
        repl();
    }
    else
    {
        char *content = NULL;
        if ((ret = read_file(argv[1], &content)) == 0)
        {
            ret = from_file(content);
        }

        free(content);
    }

    vm_free(&vm);
    return ret;
}