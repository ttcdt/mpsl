/*

    MPSL - Minimum Profit Scripting Language
    Debugging functions

    ttcdt <dev@triptico.com> et al.

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#include "mpdm.h"
#include "mpsl.h"

/** code **/

static wchar_t *dump_string(const mpdm_t v, wchar_t * ptr, int *size)
/* dumps a string, escaping special chars */
{
    wchar_t *iptr = mpdm_string(v);

    ptr = mpdm_pokews(ptr, size, L"\"");

    while (*iptr != L'\0') {
        switch (*iptr) {
        case '"':
            ptr = mpdm_pokews(ptr, size, L"\\\"");
            break;

        case '\'':
            ptr = mpdm_pokews(ptr, size, L"\\'");
            break;

        case '\r':
            ptr = mpdm_pokews(ptr, size, L"\\r");
            break;

        case '\n':
            ptr = mpdm_pokews(ptr, size, L"\\n");
            break;

        case '\t':
            ptr = mpdm_pokews(ptr, size, L"\\t");
            break;

        case '\\':
            ptr = mpdm_pokews(ptr, size, L"\\\\");
            break;

        default:
            if (*iptr >= 127) {
                char tmp[16];
                wchar_t wtmp[16];

                sprintf(tmp, "\\x{%x}", (int) *iptr);
                mbstowcs(wtmp, tmp, sizeof(wtmp));
                ptr = mpdm_pokews(ptr, size, wtmp);
            }
            else
                ptr = mpdm_pokewsn(ptr, size, iptr, 1);

            break;
        }
        iptr++;
    }

    ptr = mpdm_pokews(ptr, size, L"\"");

    return ptr;
}


wchar_t *mpsl_dump_1(const mpdm_t v, int l, wchar_t *ptr, int *size)
/* dump plugin for mpdm_dump() */
{
    int n;
    char tmp[256];
    mpdm_t w, i;
    int c, f;

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
        ptr = mpdm_pokews(ptr, size, L"NULL");
        break;

    case MPDM_TYPE_OBJECT:
        ptr = mpdm_pokews(ptr, size, L"{");

        if (mpdm_count(v)) {
            ptr = mpdm_pokews(ptr, size, L"\n");

            c = n = 0;
            while (mpdm_iterator(v, &c, &w, &i)) {
                if (n++)
                    ptr = mpdm_pokews(ptr, size, L",\n");

                for (f = 0; f <= l; f++)
                    ptr = mpdm_pokews(ptr, size, L"  ");

                ptr = mpsl_dump_1(i, l + 1, ptr, size);
                ptr = mpdm_pokews(ptr, size, L" => ");
                ptr = mpsl_dump_1(w, l + 1, ptr, size);
            }

            ptr = mpdm_pokews(ptr, size, L"\n");

            for (f = 0; f < l; f++)
                ptr = mpdm_pokews(ptr, size, L"  ");
        }

        ptr = mpdm_pokews(ptr, size, L"}");
        break;

    case MPDM_TYPE_ARRAY:
    case MPDM_TYPE_PROGRAM:
        ptr = mpdm_pokews(ptr, size, L"[");

        if (mpdm_count(v)) {
            ptr = mpdm_pokews(ptr, size, L"\n");

            c = n = 0;
            while (mpdm_iterator(v, &c, &w, NULL)) {
                if (n++)
                    ptr = mpdm_pokews(ptr, size, L",\n");

                for (f = 0; f <= l; f++)
                    ptr = mpdm_pokews(ptr, size, L"  ");

                ptr = mpsl_dump_1(w, l + 1, ptr, size);
            }

            ptr = mpdm_pokews(ptr, size, L"\n");

            for (f = 0; f < l; f++)
                ptr = mpdm_pokews(ptr, size, L"  ");
        }

        ptr = mpdm_pokews(ptr, size, L"]");
        break;

    case MPDM_TYPE_FUNCTION:
        snprintf(tmp, sizeof(tmp), "bincall('%p')", v->data);
        ptr = mpdm_pokev(ptr, size, MPDM_MBS(tmp));

        break;

    case MPDM_TYPE_INTEGER:
    case MPDM_TYPE_REAL:
        ptr = mpdm_pokews(ptr, size, mpdm_string(v));
        break;

    case MPDM_TYPE_STRING:
        ptr = dump_string(v, ptr, size);
        break;

    default:
        ptr = mpdm_pokews(ptr, size, L"NULL /" "* ");
        ptr = mpdm_pokews(ptr, size, mpdm_string(v));
        ptr = mpdm_pokews(ptr, size, L" */");

        break;
    }

    if (l == 0)
        ptr = mpdm_pokews(ptr, size, L";\n");

    return ptr;
}


static wchar_t *decompile_1(mpdm_t ins, wchar_t *ptr, int *z, mpdm_t op, int i)
{
    int n;
    mpdm_t o, v;

    /* indent */
    for (n = 0; n < i; n++)
        ptr = mpdm_pokews(ptr, z, L"  ");

    mpdm_ref(op);

    o = mpdm_get_i(ins, 0);

    /* search opcode name */
    v = mpdm_get(op, o);

    ptr = mpdm_pokev(ptr, z, v);
    ptr = mpdm_pokews(ptr, z, L"(");

    if (mpdm_ival(o) == 0) {
        /* literal */
        ptr = mpsl_dump_1(mpdm_get_i(ins, 1), -1000, ptr, z);
    }
    else {
        ptr = mpdm_pokews(ptr, z, L"\n");

        for (n = 1; n < mpdm_size(ins); n++) {
            if (n > 1)
                ptr = mpdm_pokews(ptr, z, L",\n");

            ptr = decompile_1(mpdm_get_i(ins, n), ptr, z, op, i + 1);
        }
    }

    ptr = mpdm_pokews(ptr, z, L")");

    mpdm_unref(op);

    return ptr;
}


mpdm_t mpsl_decompile(mpdm_t prg)
{
    wchar_t *ptr = NULL;
    int z = 0;
    mpdm_t op;

    if (mpdm_type(prg) == MPDM_TYPE_PROGRAM) {
        /* get the opcodes and reverse it */
        op = mpdm_omap(mpsl_opcodes, NULL, NULL);

        ptr = decompile_1(mpdm_get_i(prg, 1), ptr, &z, op, 0);
    }

    return ptr ? MPDM_NS(ptr, z) : NULL;
}
