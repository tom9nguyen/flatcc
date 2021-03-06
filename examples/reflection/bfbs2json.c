#include "support/readfile.h"
#include "flatcc/reflection/reflection_reader.h"

#define lld(x) (long long int)(x)

/*
 * Not really sure what the use case is, so printing schema to json
 * is left as a test scenario.
 */

void print_type(reflection_Type_table_t T)
{
    int first = 1;
    printf("{");
    if (reflection_Type_base_type_is_present(T)) {
        if (!first) {
            printf(",");
        }
        printf("\"base_type\":\"%s\"", reflection_BaseType_name(reflection_Type_base_type(T)));
        first = 0;
    }
    if (reflection_Type_element_is_present(T)) {
        if (!first) {
            printf(",");
        }
        printf("\"element\":\"%s\"", reflection_BaseType_name(reflection_Type_base_type(T)));
        first = 0;
    }
    if (reflection_Type_index_is_present(T)) {
        if (!first) {
            printf(",");
        }
        printf("\"index\":%d", reflection_Type_index(T));
        first = 0;
    }
    printf("}");
}

void print_object(reflection_Object_table_t O)
{
    reflection_Field_vec_t Flds;
    reflection_Field_table_t F;
    int i;

    Flds = reflection_Object_fields(O);
    printf("{\"name\":\"%s\"", reflection_Object_name(O));
    printf(",\"fields\":[");
    for (i = 0; i < reflection_Field_vec_len(Flds); ++i) {
        if (i > 0) {
            printf(",");
        }
        F = reflection_Field_vec_at(Flds, i);
        printf("{\"name\":\"%s\",\"type\":", reflection_Field_name(F));
        print_type(reflection_Field_type(F));
        if (reflection_Field_id_is_present(F)) {
            printf(",\"id\":%hu", reflection_Field_id(F));
        }
        if (reflection_Field_default_integer_is_present(F)) {
            printf(",\"default_integer\":%lld", lld(reflection_Field_default_integer(F)));
        }
        if (reflection_Field_default_real_is_present(F)) {
            printf(",\"default_integer\":%lf", reflection_Field_default_real(F));
        }
        if (reflection_Field_required_is_present(F)) {
            printf(",\"required\":%s", reflection_Field_required(F) ? "true" : "false");
        }
        if (reflection_Field_key_is_present(F)) {
            printf(",\"key\":%s", reflection_Field_key(F) ? "true" : "false");
        }
        printf("}");
    }
    printf("]");
    if (reflection_Object_is_struct_is_present(O)) {
        printf(",\"is_struct\":%s", reflection_Object_is_struct(O) ? "true" : "false");
    }
    if (reflection_Object_minalign_is_present(O)) {
        printf(",\"minalign\":%d", reflection_Object_minalign(O));
    }
    if (reflection_Object_bytesize_is_present(O)) {
        printf(",\"bytesize\":%d", reflection_Object_bytesize(O));
    }
    printf("}");
}

void print_enum(reflection_Enum_table_t E)
{
    reflection_EnumVal_vec_t EnumVals;
    reflection_EnumVal_table_t EV;
    int i;

    printf("{\"name\":\"%s\"", reflection_Enum_name(E));
    EnumVals = reflection_Enum_values(E);
    printf(",\"values\":[");
    for (i = 0; i < reflection_Enum_vec_len(EnumVals); ++i) {
        EV = reflection_EnumVal_vec_at(EnumVals, i);
        if (i > 0) {
            printf(",");
        }
        printf("{\"name\":\"%s\"", reflection_EnumVal_name(EV));
        if (reflection_EnumVal_value_is_present(EV)) {
            printf(",\"value\":%lld", lld(reflection_EnumVal_value(EV)));
        }
        if (reflection_EnumVal_object_is_present(EV)) {
            printf(",\"object\":");
            print_object(reflection_EnumVal_object(EV));
        }
        printf("}");
    }
    printf("]");
    if (reflection_Enum_is_union_is_present(E)) {
        printf(",\"is_union\":%s", reflection_Enum_is_union ? "true" : "false");
    }
    printf(",\"underlying_type\":");
    print_type(reflection_Enum_underlying_type(E));
    printf("}");
}

void print_schema(reflection_Schema_table_t S)
{
    reflection_Object_vec_t Objs;
    reflection_Enum_vec_t Enums;
    int i;

    Objs = reflection_Schema_objects(S);
    printf("{");
    printf("\"objects\":[");
    for (i = 0; i < reflection_Object_vec_len(Objs); ++i) {
        if (i > 0) {
            printf(",");
        }
        print_object(reflection_Object_vec_at(Objs, i));
    }
    printf("]");
    Enums = reflection_Schema_enums(S);
    printf(",\"enums\":[");
    for (i = 0; i < reflection_Enum_vec_len(Enums); ++i) {
        if (i > 0) {
            printf(",");
        }
        print_enum(reflection_Enum_vec_at(Enums, i));
    }
    printf("]");
    if (reflection_Schema_file_ident_is_present(S)) {
        printf(",\"file_ident\":\"%s\"", reflection_Schema_file_ident(S));
    }
    if (reflection_Schema_file_ext_is_present(S)) {
        printf(",\"file_ext\":\"%s\"", reflection_Schema_file_ext(S));
    }
    if (reflection_Schema_root_table_is_present(S)) {
        printf(",\"root_table\":");
        print_object(reflection_Schema_root_table(S));
    }
    printf("}\n");
}

int load_and_dump_schema(const char *filename)
{
    void *buffer;
    size_t size;
    int ret = -1;
    reflection_Schema_table_t S;

    buffer = read_file(filename, 10000, &size);
    if (!buffer) {
        fprintf(stderr, "failed to load binary schema file: '%s'\n", filename);
        goto done;
    }
    if (size < 12) {
        fprintf(stderr, "file too small to access: '%s'\n", filename);
        goto done;
    }
    S = reflection_Schema_as_root(buffer);
    if (!S) {
        S = reflection_Schema_as_root((char*)buffer + 4);
        if (S) {
            fprintf(stderr, "(skipping length field of input buffer)\n");
        }
    }
    if (!S) {
        fprintf(stderr, "input is not a valid schema");
        goto done;
    }
    print_schema(S);
    ret = 0;

done:
    if (buffer) {
        free(buffer);
    }
    return ret;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: bfbs2json <filename>\n");
        fprintf(stderr, "reads a binary flatbuffer schema and prints it to compact json on stdout\n\n");
        fprintf(stderr, "pretty print with exernal tool, for example:\n"
               "  bfbs2json myschema.bfbs | python -m json.tool > myschema.json\n"
               "note: also understands binary schema files with a 4 byte length prefix\n");
        exit(-1);
    }
    load_and_dump_schema(argv[1]);
}
