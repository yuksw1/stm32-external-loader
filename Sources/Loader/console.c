#include <stdarg.h>
#include <inttypes.h>

#pragma section=".bss"

//#define SUPPORT_64BIT
//#define SUPPORT_FLOAT

/**************************************************************************************************/
/* Definitions                                                                                    */
/**************************************************************************************************/
#define is_digit(c)     ((c) >= '0' && (c) <= '9')
#define to_lower(c)     ((c) | 0x20)

#ifdef SUPPORT_64BIT
    #define utype_t  uint64_t
    #define stype_t  int64_t
#else
    #define utype_t  unsigned long
    #define stype_t  signed long
#endif

/**************************************************************************************************/
/* Typedef                                                                                        */
/**************************************************************************************************/
typedef struct
{
    struct
    {
        unsigned long_flag     :2;
        unsigned lower_char    :1;
        unsigned left_flag     :1;
        unsigned sign_flag     :1;
        unsigned negative      :1;
        unsigned signed_data   :1;
        unsigned clipping      :1;
        unsigned unused        :2;
    } flags;
    char                pad_char;
    unsigned short      space_flag;
    unsigned            win_len;
    union {
        utype_t i;
#ifdef SUPPORT_FLOAT
        double  d;
#endif
    } val;
} vpf_info;

/**************************************************************************************************/
/* Global Variable Declaration                                                                    */
/**************************************************************************************************/
void UART_send_byte (uint8_t c);

/**************************************************************************************************/
/* Static Variable Declaration                                                                    */
/**************************************************************************************************/
static vpf_info s_nfo;
static const char digits[] = "0123456789ABCDEF";

/**************************************************************************************************/
/* Static Function Declaration                                                                    */
/**************************************************************************************************/
static void con_out_char (char c);
static void mem_out_char (char c);

static char *mem_ptr;
static void (*out_char) (char) = con_out_char;

/**************************************************************************************************/
/* Global Function Definition                                                                     */
/**************************************************************************************************/

void init_console (void)
{
}

void console_enter (void)
{
}

void console_leave (void)
{
}

static void mem_out_char (char c)
{
    mem_ptr[0] = c;
    mem_ptr[1] = '\0';
    mem_ptr++;
}

static void _out_char (char c)
{
    UART_send_byte ((uint8_t)c);
}

static void con_out_char (char c)
{
    if (c == '\n') {
        _out_char ('\r');
    }
    _out_char (c);
}

static unsigned my_strlen (char *str)
{
    unsigned len = 0;

    while (*str++) {
        len++;
    }
    return len;
}

static void out_str (char *str, unsigned slen)
{
    unsigned i;

    if (!s_nfo.flags.left_flag) {
        if (slen < s_nfo.win_len) {
            for (i = slen; i < s_nfo.win_len; i++) {
                out_char (s_nfo.pad_char);
            }
        }
    }
    while (*str) {
        out_char (*str++);
    }
    if (s_nfo.flags.left_flag) {
        if (slen < s_nfo.win_len) {
            for (i = slen; i < s_nfo.win_len; i++) {
                out_char (s_nfo.pad_char);
            }
        }
    }
}

static void outnum (unsigned char base)
{
    union {
        stype_t s;
        utype_t u;
    } num;
    unsigned char i;
    char outbuf[21]; /* 64bit max = 20 digit : 18446744073709551600 */

    num.u = (utype_t)s_nfo.val.i;
    if (s_nfo.flags.signed_data) {
        num.s = (stype_t)s_nfo.val.i;
        if (num.s < 0L) {
            s_nfo.flags.negative = 1;
            num.s = -num.s;
        } else {
            s_nfo.flags.negative = 0;
        }
    }

    i = sizeof (outbuf) - 1;
    outbuf[i] = '\0';
    do {
        i--;
        outbuf[i] = digits[(int) (num.u % base)];
        if (s_nfo.flags.lower_char) {
            outbuf[i] = (char) to_lower (outbuf[i]);
        }
    } while ((num.u /= base) > 0);

    if (s_nfo.flags.signed_data) {
        if (s_nfo.flags.negative) {
            i--;
            outbuf[i] = '-';
        } else if (s_nfo.flags.sign_flag) {
            i--;
            outbuf[i] = '+';
        }
    }
    out_str (&outbuf[i], my_strlen (&outbuf[i]));
}

static unsigned getnum (const char **linep)
{
    unsigned n;
    const char *cp;

    n = 0;
    cp = *linep;
    while (is_digit (*cp)) {
        n = n * 10 + (unsigned)((*cp++) - '0');
    }
    *linep = cp - 1;
    return (n);
}

static void disp_duxo (char ch)
{
    unsigned char radix;

    if ((ch == 'x') || (ch == 'X')) {
        if (ch == 'x') {
            s_nfo.flags.lower_char = 1;
        } else {
            s_nfo.flags.lower_char = 0;
        }
        radix = 16;
    } else if (ch == 'o') {
        radix = 8;
    } else {
        radix = 10;
    }

    if ((ch == 'd') || (ch == 'i')) {
        if ((stype_t)s_nfo.val.i < 0) {
            if (s_nfo.space_flag > 0) {
                s_nfo.space_flag--;
            }
        }
        while (s_nfo.space_flag--) {
            out_char (' ');
        }
        s_nfo.flags.signed_data = 1;
        outnum (radix);
    } else {
        s_nfo.flags.signed_data = 0;
        outnum (radix);
    }
}

#ifdef SUPPORT_FLOAT
static void disp_float (void)
{
    double d;
    int i;

    d = s_nfo.val.d;
    s_nfo.val.i = (stype_t)d;

    disp_duxo ('d');
    out_char ('.');
    if (d > 0)
    {
        d -= s_nfo.val.i;
    }
    else
    {
        d *= -1;
        d -= s_nfo.val.i * -1;
    }
    for (i = 0; i < 6; i++)
    {
        d *= 10;
        out_char ((char)('0' + (char)d));
        d -= (int)d;
    }
}
#endif

static void INT_vprintf (const char *format, va_list argp)
{
    char buf[2];

/*
    if (g_disable_console_out)
    {
        return;
    }
*/
    for (;;) {
        for (;;) {
            buf[0] = *format;
            if (buf[0] == '\0') {
                return;
            }

            if (buf[0] != '%') {
                out_char (buf[0]);
            } else {
                break;
            }
            format++;
        }

        s_nfo.win_len = 0;
        s_nfo.flags.long_flag = 0;
        s_nfo.flags.left_flag = 0;
        s_nfo.flags.sign_flag = 0;
        s_nfo.flags.clipping = 0;
        s_nfo.space_flag = 0;
        s_nfo.pad_char = ' ';

        for (;;) {
            format++;
            buf[0] = *format;
            if (buf[0] == '\0') {
                return;
            }

            if (is_digit (buf[0])) {
                if (buf[0] == '0') {
                    s_nfo.pad_char = '0';
                }
                s_nfo.win_len = getnum (&format);
            } else if (buf[0] == '%') {
                out_char (buf[0]);
                format++;
                break;
            } else if (buf[0] == '-') {
                s_nfo.flags.left_flag = 1;
            } else if (buf[0] == '+') {
                s_nfo.flags.sign_flag = 1;
            } else if ((buf[0] == 'l') || (buf[0] == 'L')) {
                s_nfo.flags.long_flag++;
            } else if (buf[0] == ' ') {
                s_nfo.space_flag++;
#ifdef SUPPORT_FLOAT
            } else if (buf[0] == 'f') {
                s_nfo.val.d = (double) va_arg (argp, double);
                disp_float ();
                format++;
                break;
#endif
            } else if ((buf[0] == 'd') || (buf[0] == 'i') || (buf[0] == 'u') || (buf[0] == 'x') || (buf[0] == 'X') || (buf[0] == 'o') || (buf[0] == 'p')) {
                if (s_nfo.flags.long_flag == 0) {
                    if ((buf[0] == 'd') || (buf[0] == 'i'))
                    {
                        s_nfo.val.i = (stype_t) va_arg (argp, int);
                    }
                    else
                    {
                        s_nfo.val.i = (utype_t) va_arg (argp, unsigned int);
                    }
                }
                else if (s_nfo.flags.long_flag == 1) {
                    s_nfo.val.i = (utype_t)va_arg (argp, unsigned long);
                } else if (s_nfo.flags.long_flag == 2) {
                    s_nfo.val.i = (uint64_t)va_arg (argp, uint32_t);
                    s_nfo.val.i |= (uint64_t)va_arg (argp, uint32_t) << 32;
                }
                if (buf[0] == 'p') {
                    s_nfo.pad_char = '0';
                    s_nfo.win_len = sizeof(void *) * 2;
                    buf[0] = 'X';
                }
                disp_duxo (buf[0]);
                format++;
                break;
            } else if (buf[0] == 's') {
                char *ppp;

                ppp = va_arg (argp, char *);
                out_str (ppp, my_strlen (ppp));
                format++;
                break;
            } else if (buf[0] == 'c') {
                buf[0] = (char)va_arg (argp, int);
                buf[1] = '\0';
                out_str (buf, 1);
                format++;
                break;
            }
        }
    }
}

/************************************************************************************/
static void P_vprintf (const char *format, va_list argp)
{
    INT_vprintf (format, argp);
}

void c_vprintf (const char *format, va_list argp)
{
    P_vprintf (format, argp);
}

int g_disable_c_printf;

void c_printf (const char *format, ...)
{
    va_list argp;

    if (g_disable_c_printf) return;

    console_enter ();
    va_start (argp, format);
    c_vprintf (format, argp);
    va_end (argp);
    console_leave ();
}

static int INT_c_vsprintf (char *buf, const char *format, va_list argp)
{
    mem_ptr = buf;
    out_char = mem_out_char;
    c_vprintf (format, argp);
    out_char = con_out_char;
    return (int)(mem_ptr - buf);
}

int c_vsprintf (char *buf, const char *format, va_list argp)
{
    int ret;

    console_enter ();
    ret = INT_c_vsprintf (buf, format, argp);
    console_leave ();

    return ret;
}

int c_sprintf(char *buf, const char *format, ...)
{
    int len;
    va_list argp;

    console_enter ();
    va_start (argp, format);
    len = INT_c_vsprintf (buf, format, argp);
    va_end (argp);
    console_leave ();
    return len;
}
