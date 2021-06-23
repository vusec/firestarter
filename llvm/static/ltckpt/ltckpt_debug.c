#ifdef __MINIX
#include <lib.h>
#include <stdarg.h>
#include <minix/com.h>
#include <minix/syslib.h>
#include <minix/sysutil.h>

#include "ltckpt_local.h"
#include "ltckpt_debug.h"

int lt_kernel_call(int syscallnr, message *msgptr)
{
  int t, r;
  t = 1;
  while(1) {
      msgptr->m_type = syscallnr;
      do_kernel_call(msgptr);
      r = msgptr->m_type;
      if(r != ENOTREADY) {
          break;
      }
  }
  return r;
}

static void lt_kputs(char *buf)
{
  message m;
  m.m_lsys_krn_sys_diagctl.code = DIAGCTL_CODE_DIAG;
  m.m_lsys_krn_sys_diagctl.buf = (vir_bytes) buf;
  m.m_lsys_krn_sys_diagctl.len = strlen(buf);
  lt_kernel_call(SYS_DIAGCTL, &m);
}

void lt_kputc(char c)
{
  message m;
  m.m_lsys_krn_sys_diagctl.code = DIAGCTL_CODE_DIAG;
  m.m_lsys_krn_sys_diagctl.buf = (vir_bytes) &c;
  m.m_lsys_krn_sys_diagctl.len = 1;
  lt_kernel_call(SYS_DIAGCTL, &m);
}

static char static_malloc_buf[100000];
static int static_malloc_used = 0;

static void lt_kputn(unsigned int n)
{
	if(n >= 10) { lt_kputn(n/10); }
	lt_kputc('0' + n % 10);
}

static void lt_kputx_(unsigned int n)
{
	const char *x = "0123456789abcdef";
	if(n >= 16) { lt_kputx_(n/16); }
	lt_kputc(x[n % 16]);
}

static void lt_kputx(unsigned int n)
{
	lt_kputc('0');
	lt_kputc('x');
	lt_kputx_(n);
}

void *lt_malloc(int size)
{
	void *r = static_malloc_buf + static_malloc_used;
	static_malloc_used += size;
	lt_assert(static_malloc_used <= sizeof(static_malloc_buf));
	return r;
}

void lt_free(void *mem)
{

}

void lt_printf(const char *fmt, ...)
{
	/* Primitive printf() that just understands %s, %d, %x
	 * and can be called safely from within writelog-writing code.
	 */

	va_list ap;
	va_start(ap, fmt);
	unsigned char c;
	while((c=(*fmt)) != '\0') {
		unsigned int n_arg;
		char *s_arg;
		fmt++;
		if(c != '%') { lt_kputc(c); continue; }
		c = *fmt; fmt++;
		while(c == 'l') {
			/* skip 'long' indicator */
			c = *fmt; fmt++;
		}
		switch(c) {
			case 'u':
			case 'd':
				n_arg = va_arg(ap, int);
				lt_kputn(n_arg);
				break;
			case 'x':
				n_arg = va_arg(ap, int);
				lt_kputx(n_arg);
				break;
			case 's':
				s_arg = va_arg(ap, char *);
				lt_kputs(s_arg);
				break;
		}
	}
	va_end(ap);
}

#endif

