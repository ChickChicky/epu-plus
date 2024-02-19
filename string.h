#ifndef string_h
#define string_h

/* Compares two lengthed strings */
int strcmpl(const char* a, const char* b, int l) 
#ifdef string_impl
{
    for (int i = 0; i < l; i++) if (a[i] != b[i]) return 1;
    return 0;
}
#endif
;

#endif