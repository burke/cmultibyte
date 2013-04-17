#include "ruby.h"
#include <stdint.h>
#include <stdio.h>

VALUE ActiveSupport = Qnil;
VALUE Multibyte = Qnil;
VALUE Unicode = Qnil;
int *cp1252;

static ID idUnpack;
static ID idPack;
static ID idForceEncoding;

static int inline is_cont(uint8_t byte)       { return (byte > 127 && byte < 192); }
static int inline is_lead(uint8_t byte)       { return (byte > 191 && byte < 245); }
static int inline is_unused(uint8_t byte)     { return (byte > 240); }
static int inline is_restricted(uint8_t byte) { return (byte > 244); }

static uint8_t *tidy_byte2(uint8_t byte, uint8_t *dst) {
  int map, i;
  VALUE ary;

  if (byte < 160) {
    map = cp1252[byte];
    if (map == -1) {
      map = byte;
    }
    ary = rb_ary_new2(1);
    rb_ary_store(ary, 0, INT2FIX(map));
    ary = rb_funcall(ary, idPack, 1, rb_str_new2("U"));
    ary = rb_funcall(ary, idUnpack, 1, rb_str_new2("C*"));
    for (i = 0; i < RARRAY_LEN(ary) ; i++) {
      dst[i] = FIX2INT(RARRAY_PTR(ary)[i]);
    }
    return dst + RARRAY_LEN(ary);
  } else if (byte < 192) {
    dst[0] = 194;
    dst[1] = byte;
    return dst + 2;
  } else {
    dst[0] = 195;
    dst[1] = byte - 64;
    return dst + 2;
  }
}

VALUE tidy_byte(uint8_t byte) {
  int map;
  VALUE ary;

  if (byte < 160) {
    map = cp1252[byte];
    if (map == -1) {
      map = byte;
    }
    ary = rb_ary_new2(1);
    rb_ary_store(ary, 0, INT2FIX(map));
    ary = rb_funcall(ary, idPack, 1, rb_str_new2("U"));
    ary = rb_funcall(ary, idUnpack, 1, rb_str_new2("C*"));
    return ary;
  } else if (byte < 192) {
    ary = rb_ary_new2(2);
    rb_ary_store(ary, 0, INT2FIX(194));
    rb_ary_store(ary, 1, INT2FIX(byte));
    return ary;
  } else {
    ary = rb_ary_new2(2);
    rb_ary_store(ary, 0, INT2FIX(195));
    rb_ary_store(ary, 1, INT2FIX(byte - 64));
    return ary;
  }
}

VALUE rb_tidy_byte(VALUE self, VALUE byte) {
  return tidy_byte(FIX2INT(byte));
}

static void cp1252_hash_to_array(VALUE cp1252_hash) {
  int i;
  cp1252 = ALLOC_N(int, 256);
  VALUE val;
  for (i = 0; i < 256; i++) {
    val = rb_hash_aref(cp1252_hash, INT2FIX(i));
    if (val == Qnil) {
      cp1252[i] = -1;
    } else {
      cp1252[i] = FIX2INT(val);
    }
  }
}

struct protected_tidy_bytes_arg {
  VALUE string;
  uint8_t *arr;
};

static VALUE protected_tidy_bytes(VALUE arg) {
  struct protected_tidy_bytes_arg* args = (struct protected_tidy_bytes_arg *)arg;
  VALUE string = args->string;
  uint8_t *arr = args->arr;

  int conts_expected = 0;
  int i, j;
  uint8_t byte;

  uint8_t *curr = arr;
  uint8_t *last_lead = curr;

  int did_write = 0;

  for (i = 0; i < RSTRING_LEN(string) ; i++) {
    byte = RSTRING_PTR(string)[i];
    curr[0] = byte;
    did_write = 0;

    if (is_unused(byte) || is_restricted(byte)) {
      curr = tidy_byte2(byte, curr);
      did_write = 1;
    } else if (is_cont(byte)) {
      if (conts_expected == 0) {
        curr = tidy_byte2(byte, curr);
        did_write = 1;
      } else {
        conts_expected--;
      }
    } else {
      if (conts_expected > 0) {
        int start_index = last_lead-arr;
        int end_index = curr-arr;
        int len = end_index - start_index;

        uint8_t temp[len];
        for (j = 0; j < len; j++) {
          temp[j] = arr[start_index + j];
        }

        curr = arr + start_index;
        for (j = 0; j < len; j++) {
          curr = tidy_byte2(temp[j], curr);
        }

        conts_expected = 0;
      }
      if (is_lead(byte)) {
        if (i == RSTRING_LEN(string) - 1) {
          curr = tidy_byte2(byte, curr);
          did_write = 1;
        } else {
          if (byte < 224) {
            conts_expected = 1;
          } else if (byte < 240) {
            conts_expected = 2;
          } else {
            conts_expected = 3;
          }
          last_lead = curr;
        }
      }
    }
    if (!did_write) {
      *curr++ = byte;
    }
  }
  VALUE str = rb_str_new((const char *)arr, curr-arr);
  return rb_funcall(str, idForceEncoding, 1, rb_str_new2("UTF-8"));
}

static VALUE tidy_bytes(VALUE string) {
  uint8_t *arr = ALLOC_N(uint8_t, 4 * RSTRING_LEN(string));
  VALUE ret;
  int exception = 0;
  struct protected_tidy_bytes_arg args;

  args.string = string;
  args.arr = arr;

  ret = rb_protect(protected_tidy_bytes, (VALUE)&args, &exception);

  xfree(arr);
  if (exception) {
    rb_jump_tag(exception);
  }
  return ret;
}


static VALUE force_tidy_bytes(VALUE string) {
  uint8_t *arr = malloc(4 * sizeof(uint8_t) * RSTRING_LEN(string));
  uint8_t *curr = arr;
  int i;
  for (i = 0 ; i < RSTRING_LEN(string) ; i++) {
    curr = tidy_byte2(RSTRING_PTR(string)[i], curr);
  }
  VALUE str = rb_str_new((const char *)arr, curr - arr);
  free(arr);
  return str;
}

VALUE rb_tidy_bytes(int argc, VALUE *argv, VALUE self) {
  VALUE string, force;
  rb_scan_args(argc, argv, "11", &string, &force);
  if (force != Qfalse && force != Qnil) {
    return force_tidy_bytes(string);
  } else {
    return tidy_bytes(string);
  }
}


void Init_cmultibyte() {
  idUnpack = rb_intern("unpack");
  idPack = rb_intern("pack");
  idForceEncoding = rb_intern("force_encoding");

  rb_funcall(rb_cObject, rb_intern("require"), 1, rb_str_new2("active_support/all"));

  ActiveSupport = rb_const_get(rb_cObject, rb_intern("ActiveSupport"));
  Multibyte = rb_const_get_at(ActiveSupport, rb_intern("Multibyte"));
  Unicode = rb_const_get_at(Multibyte, rb_intern("Unicode"));

  VALUE database = rb_funcall(Unicode, rb_intern("database"), 0);
  rb_funcall(database, rb_intern("load"), 0);
  VALUE cp1252_hash = rb_ivar_get(database, rb_intern("@cp1252"));
  cp1252_hash_to_array(cp1252_hash);

  rb_define_singleton_method(Unicode, "tidy_byte", rb_tidy_byte, 1);
  rb_define_singleton_method(Unicode, "tidy_bytes", rb_tidy_bytes, -1);
}
