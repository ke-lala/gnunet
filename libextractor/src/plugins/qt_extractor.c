/*
     This file is part of libextractor.
     Copyright (C) 2002, 2003, 2006, 2012, 2026 Vidyut Samanta and Christian Grothoff

     libextractor is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     libextractor is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libextractor; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
     Boston, MA 02110-1301, USA.
 */
/**
 * @file plugins/qt_extractor.c
 * @brief plugin to support QuickTime, MP4, M4A and 3GPP files
 * @author Vidyut Samanta
 * @author Christian Grothoff
 *
 * This plugin parses the ISO base media / QuickTime "atom" (box) tree.
 * It does not link against any third-party demuxer: the metadata-bearing
 * atoms ('ftyp', 'moov' and the boxes nested inside it) are tiny compared
 * to the media payload ('mdat'), so the plugin streams the top-level
 * boxes via the extraction context and only ever pulls the small
 * metadata containers into memory before walking them recursively.
 */
#include "platform.h"
#include "extractor.h"
#include <zlib.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Maximum size (in bytes) of a single top-level atom that we are willing
 * to pull into memory.  'moov' is always far smaller than this in
 * practice; the cap merely protects us against hostile or corrupt files.
 */
#define MAX_ATOM_SIZE (64 * 1024 * 1024)

/**
 * Maximum size of a (decompressed) compressed-movie 'cmov' atom.
 */
#define MAX_CMOV_SIZE (16 * 1024 * 1024)

/**
 * Maximum atom nesting depth we are willing to recurse into.  Real files
 * stay well below ten; the limit guards against stack exhaustion from
 * maliciously deeply nested boxes.
 */
#define MAX_ATOM_DEPTH 32


/* verbatim from mp3extractor */
static const char *const genre_names[] = {
  gettext_noop ("Blues"),
  gettext_noop ("Classic Rock"),
  gettext_noop ("Country"),
  gettext_noop ("Dance"),
  gettext_noop ("Disco"),
  gettext_noop ("Funk"),
  gettext_noop ("Grunge"),
  gettext_noop ("Hip-Hop"),
  gettext_noop ("Jazz"),
  gettext_noop ("Metal"),
  gettext_noop ("New Age"),
  gettext_noop ("Oldies"),
  gettext_noop ("Other"),
  gettext_noop ("Pop"),
  gettext_noop ("R&B"),
  gettext_noop ("Rap"),
  gettext_noop ("Reggae"),
  gettext_noop ("Rock"),
  gettext_noop ("Techno"),
  gettext_noop ("Industrial"),
  gettext_noop ("Alternative"),
  gettext_noop ("Ska"),
  gettext_noop ("Death Metal"),
  gettext_noop ("Pranks"),
  gettext_noop ("Soundtrack"),
  gettext_noop ("Euro-Techno"),
  gettext_noop ("Ambient"),
  gettext_noop ("Trip-Hop"),
  gettext_noop ("Vocal"),
  gettext_noop ("Jazz+Funk"),
  gettext_noop ("Fusion"),
  gettext_noop ("Trance"),
  gettext_noop ("Classical"),
  gettext_noop ("Instrumental"),
  gettext_noop ("Acid"),
  gettext_noop ("House"),
  gettext_noop ("Game"),
  gettext_noop ("Sound Clip"),
  gettext_noop ("Gospel"),
  gettext_noop ("Noise"),
  gettext_noop ("Alt. Rock"),
  gettext_noop ("Bass"),
  gettext_noop ("Soul"),
  gettext_noop ("Punk"),
  gettext_noop ("Space"),
  gettext_noop ("Meditative"),
  gettext_noop ("Instrumental Pop"),
  gettext_noop ("Instrumental Rock"),
  gettext_noop ("Ethnic"),
  gettext_noop ("Gothic"),
  gettext_noop ("Darkwave"),
  gettext_noop ("Techno-Industrial"),
  gettext_noop ("Electronic"),
  gettext_noop ("Pop-Folk"),
  gettext_noop ("Eurodance"),
  gettext_noop ("Dream"),
  gettext_noop ("Southern Rock"),
  gettext_noop ("Comedy"),
  gettext_noop ("Cult"),
  gettext_noop ("Gangsta Rap"),
  gettext_noop ("Top 40"),
  gettext_noop ("Christian Rap"),
  gettext_noop ("Pop/Funk"),
  gettext_noop ("Jungle"),
  gettext_noop ("Native American"),
  gettext_noop ("Cabaret"),
  gettext_noop ("New Wave"),
  gettext_noop ("Psychedelic"),
  gettext_noop ("Rave"),
  gettext_noop ("Showtunes"),
  gettext_noop ("Trailer"),
  gettext_noop ("Lo-Fi"),
  gettext_noop ("Tribal"),
  gettext_noop ("Acid Punk"),
  gettext_noop ("Acid Jazz"),
  gettext_noop ("Polka"),
  gettext_noop ("Retro"),
  gettext_noop ("Musical"),
  gettext_noop ("Rock & Roll"),
  gettext_noop ("Hard Rock"),
  gettext_noop ("Folk"),
  gettext_noop ("Folk/Rock"),
  gettext_noop ("National Folk"),
  gettext_noop ("Swing"),
  gettext_noop ("Fast-Fusion"),
  gettext_noop ("Bebob"),
  gettext_noop ("Latin"),
  gettext_noop ("Revival"),
  gettext_noop ("Celtic"),
  gettext_noop ("Bluegrass"),
  gettext_noop ("Avantgarde"),
  gettext_noop ("Gothic Rock"),
  gettext_noop ("Progressive Rock"),
  gettext_noop ("Psychedelic Rock"),
  gettext_noop ("Symphonic Rock"),
  gettext_noop ("Slow Rock"),
  gettext_noop ("Big Band"),
  gettext_noop ("Chorus"),
  gettext_noop ("Easy Listening"),
  gettext_noop ("Acoustic"),
  gettext_noop ("Humour"),
  gettext_noop ("Speech"),
  gettext_noop ("Chanson"),
  gettext_noop ("Opera"),
  gettext_noop ("Chamber Music"),
  gettext_noop ("Sonata"),
  gettext_noop ("Symphony"),
  gettext_noop ("Booty Bass"),
  gettext_noop ("Primus"),
  gettext_noop ("Porn Groove"),
  gettext_noop ("Satire"),
  gettext_noop ("Slow Jam"),
  gettext_noop ("Club"),
  gettext_noop ("Tango"),
  gettext_noop ("Samba"),
  gettext_noop ("Folklore"),
  gettext_noop ("Ballad"),
  gettext_noop ("Power Ballad"),
  gettext_noop ("Rhythmic Soul"),
  gettext_noop ("Freestyle"),
  gettext_noop ("Duet"),
  gettext_noop ("Punk Rock"),
  gettext_noop ("Drum Solo"),
  gettext_noop ("A Cappella"),
  gettext_noop ("Euro-House"),
  gettext_noop ("Dance Hall"),
  gettext_noop ("Goa"),
  gettext_noop ("Drum & Bass"),
  gettext_noop ("Club-House"),
  gettext_noop ("Hardcore"),
  gettext_noop ("Terror"),
  gettext_noop ("Indie"),
  gettext_noop ("BritPop"),
  gettext_noop ("Negerpunk"),
  gettext_noop ("Polsk Punk"),
  gettext_noop ("Beat"),
  gettext_noop ("Christian Gangsta Rap"),
  gettext_noop ("Heavy Metal"),
  gettext_noop ("Black Metal"),
  gettext_noop ("Crossover"),
  gettext_noop ("Contemporary Christian"),
  gettext_noop ("Christian Rock"),
  gettext_noop ("Merengue"),
  gettext_noop ("Salsa"),
  gettext_noop ("Thrash Metal"),
  gettext_noop ("Anime"),
  gettext_noop ("JPop"),
  gettext_noop ("Synthpop"),
};

#define GENRE_NAME_COUNT \
        ((unsigned int) (sizeof genre_names / sizeof (const char *const)))


static const char *languages[] = {
  "English",
  "French",
  "German",
  "Italian",
  "Dutch",
  "Swedish",
  "Spanish",
  "Danish",
  "Portuguese",
  "Norwegian",
  "Hebrew",
  "Japanese",
  "Arabic",
  "Finnish",
  "Greek",
  "Icelandic",
  "Maltese",
  "Turkish",
  "Croatian",
  "Traditional Chinese",
  "Urdu",
  "Hindi",
  "Thai",
  "Korean",
  "Lithuanian",
  "Polish",
  "Hungarian",
  "Estonian",
  "Lettish",
  "Saamisk",
  "Lappish",
  "Faeroese",
  "Farsi",
  "Russian",
  "Simplified Chinese",
  "Flemish",
  "Irish",
  "Albanian",
  "Romanian",
  "Czech",
  "Slovak",
  "Slovenian",
  "Yiddish",
  "Serbian",
  "Macedonian",
  "Bulgarian",
  "Ukrainian",
  "Byelorussian",
  "Uzbek",
  "Kazakh",
  "Azerbaijani",
  "AzerbaijanAr",
  "Armenian",
  "Georgian",
  "Moldavian",
  "Kirghiz",
  "Tajiki",
  "Turkmen",
  "Mongolian",
  "MongolianCyr",
  "Pashto",
  "Kurdish",
  "Kashmiri",
  "Sindhi",
  "Tibetan",
  "Nepali",
  "Sanskrit",
  "Marathi",
  "Bengali",
  "Assamese",
  "Gujarati",
  "Punjabi",
  "Oriya",
  "Malayalam",
  "Kannada",
  "Tamil",
  "Telugu",
  "Sinhalese",
  "Burmese",
  "Khmer",
  "Lao",
  "Vietnamese",
  "Indonesian",
  "Tagalog",
  "MalayRoman",
  "MalayArabic",
  "Amharic",
  "Tigrinya",
  "Galla",
  "Oromo",
  "Somali",
  "Swahili",
  "Ruanda",
  "Rundi",
  "Chewa",
  "Malagasy",
  "Esperanto",
  "Welsh",
  "Basque",
  "Catalan",
  "Latin",
  "Quechua",
  "Guarani",
  "Aymara",
  "Tatar",
  "Uighur",
  "Dzongkha",
  "JavaneseRom",
};


typedef struct
{
  const char *ext;
  const char *mime;
} C2M;

/* see http://www.mp4ra.org/filetype.html
 *     http://www.ftyps.com/ */
static C2M ftMap[] = {
  {"qt  ", "video/quicktime"},
  {"isom", "video/mp4"},        /* ISO Base Media files */
  {"iso2", "video/mp4"},
  {"iso4", "video/mp4"},
  {"iso5", "video/mp4"},
  {"iso6", "video/mp4"},
  {"avc1", "video/mp4"},
  {"mp41", "video/mp4"},        /* MPEG-4 (ISO/IEC 14491-1) version 1 */
  {"mp42", "video/mp4"},        /* MPEG-4 (ISO/IEC 14491-1) version 2 */
  {"mp71", "video/mp4"},        /* MPEG-4 with MPEG-7 metadata */
  {"dash", "video/mp4"},        /* MPEG-DASH */
  {"3gp1", "video/3gpp"},
  {"3gp2", "video/3gpp"},
  {"3gp3", "video/3gpp"},
  {"3gp4", "video/3gpp"},
  {"3gp5", "video/3gpp"},
  {"3gp6", "video/3gpp"},
  {"3gp7", "video/3gpp"},
  {"3g2a", "video/3gpp2"},
  {"3g2b", "video/3gpp2"},
  {"3g2c", "video/3gpp2"},
  {"mmp4", "video/mp4"},        /* Mobile MPEG-4 */
  {"M4A ", "audio/mp4"},
  {"M4B ", "audio/mp4"},        /* Apple audio book */
  {"M4P ", "audio/mp4"},
  {"M4V ", "video/mp4"},
  {"M4VH", "video/mp4"},
  {"M4VP", "video/mp4"},
  {"f4v ", "video/mp4"},        /* Adobe Flash MP4 video */
  {"f4a ", "audio/mp4"},
  {"f4b ", "audio/mp4"},
  {"qt  ", "video/quicktime"},
  {"mj2s", "video/mj2"},        /* Motion JPEG 2000 */
  {"mjp2", "video/mj2"},
  {NULL, NULL},
};

typedef struct CHE
{
  const char *pfx;
  enum EXTRACTOR_MetaType type;
} CHE;

static CHE cHm[] = {
  {"aut", EXTRACTOR_METATYPE_AUTHOR_NAME},
  {"cpy", EXTRACTOR_METATYPE_COPYRIGHT},
  {"day", EXTRACTOR_METATYPE_CREATION_DATE},
  {"ed1", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed2", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed3", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed4", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed5", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed6", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed7", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed8", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"ed9", EXTRACTOR_METATYPE_MODIFICATION_DATE},
  {"cmt", EXTRACTOR_METATYPE_COMMENT},
  {"url", EXTRACTOR_METATYPE_URL},
  {"enc", EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE},
  {"hst", EXTRACTOR_METATYPE_BUILDHOST},
  {"nam", EXTRACTOR_METATYPE_TITLE},
  {"gen", EXTRACTOR_METATYPE_GENRE},
  {"mak", EXTRACTOR_METATYPE_CAMERA_MAKE},
  {"mod", EXTRACTOR_METATYPE_CAMERA_MODEL},
  {"des", EXTRACTOR_METATYPE_DESCRIPTION},
  {"dis", EXTRACTOR_METATYPE_DISCLAIMER},
  {"dir", EXTRACTOR_METATYPE_MOVIE_DIRECTOR},
  {"src", EXTRACTOR_METATYPE_CONTRIBUTOR_NAME},
  {"prf", EXTRACTOR_METATYPE_PERFORMER },
  {"prd", EXTRACTOR_METATYPE_PRODUCER},
  {"PRD", EXTRACTOR_METATYPE_PRODUCT_VERSION},
  {"swr", EXTRACTOR_METATYPE_PRODUCED_BY_SOFTWARE},
  {"isr", EXTRACTOR_METATYPE_ISRC},
  {"wrt", EXTRACTOR_METATYPE_WRITER},
  {"wrn", EXTRACTOR_METATYPE_WARNING},
  {"chp", EXTRACTOR_METATYPE_CHAPTER_NAME},
  {"inf", EXTRACTOR_METATYPE_DESCRIPTION},
  {"req", EXTRACTOR_METATYPE_TARGET_PLATFORM},      /* hardware requirements */
  {"fmt", EXTRACTOR_METATYPE_FORMAT},
  {"alb", EXTRACTOR_METATYPE_ALBUM},
  {"ART", EXTRACTOR_METATYPE_ARTIST},
  {"art", EXTRACTOR_METATYPE_ARTIST},
  {"too", EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE},
  {"grp", EXTRACTOR_METATYPE_GROUP},
  {"lyr", EXTRACTOR_METATYPE_LYRICS},
  {"st3", EXTRACTOR_METATYPE_SUBTITLE},
  {NULL, EXTRACTOR_METATYPE_RESERVED },
};


typedef struct
{
  const char *atom_type;
  enum EXTRACTOR_MetaType type;
} ITTagConversionEntry;

/* iTunes / "ilst" tags:
 * see http://atomicparsley.sourceforge.net/mpeg-4files.html
 *
 * The first byte of the four-character key is the (C) / 0xa9 sign for
 * the "user" tags; we keep it spelled out here so the table can be
 * memcmp()ed against the raw atom name. */
static ITTagConversionEntry it_to_extr_table[] = {
  {"\xa9" "alb", EXTRACTOR_METATYPE_ALBUM},
  {"\xa9" "ART", EXTRACTOR_METATYPE_ARTIST},
  {"\xa9" "art", EXTRACTOR_METATYPE_ARTIST},
  {"aART", EXTRACTOR_METATYPE_ARTIST},               /* album artist */
  {"\xa9" "cmt", EXTRACTOR_METATYPE_COMMENT},
  {"\xa9" "day", EXTRACTOR_METATYPE_UNKNOWN_DATE},
  {"\xa9" "nam", EXTRACTOR_METATYPE_TITLE},
  {"\xa9" "trk", EXTRACTOR_METATYPE_TRACK_NUMBER},
  {"trkn", EXTRACTOR_METATYPE_TRACK_NUMBER},
  {"\xa9" "dis", EXTRACTOR_METATYPE_DISC_NUMBER},
  {"disk", EXTRACTOR_METATYPE_DISC_NUMBER},
  {"\xa9" "gen", EXTRACTOR_METATYPE_GENRE},
  {"gnre", EXTRACTOR_METATYPE_GENRE},
  {"\xa9" "wrt", EXTRACTOR_METATYPE_COMPOSER},
  {"\xa9" "com", EXTRACTOR_METATYPE_COMPOSER},
  {"\xa9" "too", EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE},
  {"\xa9" "enc", EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE},
  {"cprt", EXTRACTOR_METATYPE_COPYRIGHT},
  {"\xa9" "cpy", EXTRACTOR_METATYPE_COPYRIGHT},
  {"\xa9" "grp", EXTRACTOR_METATYPE_GROUP},
  {"\xa9" "lyr", EXTRACTOR_METATYPE_LYRICS},
  {"\xa9" "st3", EXTRACTOR_METATYPE_SUBTITLE},
  {"\xa9" "url", EXTRACTOR_METATYPE_URL},
  {"\xa9" "prd", EXTRACTOR_METATYPE_PRODUCER},
  {"\xa9" "dir", EXTRACTOR_METATYPE_MOVIE_DIRECTOR},
  {"\xa9" "prf", EXTRACTOR_METATYPE_PERFORMER},
  {"\xa9" "swr", EXTRACTOR_METATYPE_PRODUCED_BY_SOFTWARE},
  {"\xa9" "fmt", EXTRACTOR_METATYPE_FORMAT},
  {"\xa9" "inf", EXTRACTOR_METATYPE_DESCRIPTION},
  {"tmpo", EXTRACTOR_METATYPE_BEATS_PER_MINUTE},
  {"catg", EXTRACTOR_METATYPE_SECTION},
  {"keyw", EXTRACTOR_METATYPE_KEYWORDS},
  {"desc", EXTRACTOR_METATYPE_DESCRIPTION},
  {"ldes", EXTRACTOR_METATYPE_DESCRIPTION},          /* long description */
  {"tvnn", EXTRACTOR_METATYPE_NETWORK_NAME},
  {"tvsh", EXTRACTOR_METATYPE_SHOW_NAME},
  {"tvsn", EXTRACTOR_METATYPE_SHOW_SEASON_NUMBER},
  {"tves", EXTRACTOR_METATYPE_SHOW_EPISODE_NUMBER},
  {"purd", EXTRACTOR_METATYPE_UNKNOWN_DATE},         /* purchase date */
  {"covr", EXTRACTOR_METATYPE_COVER_PICTURE},
  {NULL, EXTRACTOR_METATYPE_RESERVED}
};


struct Atom
{
  uint32_t size;
  uint32_t type;
};


struct LongAtom
{
  uint32_t one;
  uint32_t type;
  uint64_t size;
};


static uint64_t
ntohll (uint64_t n)
{
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return (((uint64_t) ntohl (n)) << 32) + ntohl (n >> 32);
#endif
}


/**
 * Check if at position pos there is a valid atom.
 * @return false if the atom is invalid, true if it is valid
 */
static bool
checkAtomValid (const char *buffer,
                size_t size,
                size_t pos)
{
  unsigned long long atomSize;
  const struct Atom *atom;
  const struct LongAtom *latom;

  if ( (pos >= size) ||
       (pos + sizeof (struct Atom) > size) ||
       (pos + sizeof (struct Atom) < pos) )
    return false;
  atom = (const struct Atom *) &buffer[pos];
  if (ntohl (atom->size) == 1)
  {
    if ( (pos + sizeof (struct LongAtom) > size) ||
         (pos + sizeof (struct LongAtom) < pos) )
      return false;
    latom = (const struct LongAtom *) &buffer[pos];
    atomSize = ntohll (latom->size);
    if ((atomSize < sizeof (struct LongAtom)) ||
        (atomSize + pos > size) || (atomSize + pos < atomSize))
      return false;
  }
  else
  {
    atomSize = ntohl (atom->size);
    if ((atomSize < sizeof (struct Atom)) ||
        (atomSize + pos > size) || (atomSize + pos < atomSize))
      return false;
  }
  return true;
}


/**
 * Assumes that checkAtomValid has already been called.
 */
static uint64_t
getAtomSize (const char *buf)
{
  const struct Atom *atom;
  const struct LongAtom *latom;

  atom = (const struct Atom *) buf;
  if (ntohl (atom->size) == 1)
  {
    latom = (const struct LongAtom *) buf;
    return ntohll (latom->size);
  }
  return ntohl (atom->size);
}


/**
 * Assumes that checkAtomValid has already been called.
 */
static size_t
getAtomHeaderSize (const char *buf)
{
  const struct Atom *atom;

  atom = (const struct Atom *) buf;
  if (ntohl (atom->size) == 1)
    return sizeof (const struct LongAtom);
  return sizeof (struct Atom);
}


/**
 * State carried through the recursive atom walk.
 */
struct ExtractContext
{
  /**
   * The libextractor processing callback.
   */
  EXTRACTOR_MetaDataProcessor proc;

  /**
   * Closure for @e proc.
   */
  void *proc_cls;

  /**
   * Set to non-zero once @e proc asked us to stop.
   */
  int ret;

  /**
   * Current atom nesting depth (for recursion limiting).
   */
  unsigned int depth;
};


static void
addKeyword (enum EXTRACTOR_MetaType type,
            const char *str,
            struct ExtractContext *ec)
{
  if (ec->ret != 0)
    return;
  ec->ret = ec->proc (ec->proc_cls,
                      "qt",
                      type,
                      EXTRACTOR_METAFORMAT_UTF8,
                      "text/plain",
                      str,
                      strlen (str) + 1);
}


static void
addBinary (enum EXTRACTOR_MetaType type,
           const char *mime,
           const void *data,
           size_t data_len,
           struct ExtractContext *ec)
{
  if (ec->ret != 0)
    return;
  ec->ret = ec->proc (ec->proc_cls,
                      "qt",
                      type,
                      EXTRACTOR_METAFORMAT_BINARY,
                      mime,
                      data,
                      data_len);
}


/**
 * Assumes that checkAtomValid has already been called.
 *
 * @return 0 on a fatal error (stop the current level),
 *         1 for success, -1 for "atom not understood, skip it"
 */
typedef int
(*AtomHandler) (const char *input,
                size_t size,
                size_t pos,
                struct ExtractContext *ec);

struct HandlerEntry
{
  const char *name;
  AtomHandler handler;
};


/**
 * Call the handler for the atom at the given position.
 * Will check validity of the given atom.
 *
 * @return 0 on error, 1 for success, -1 for unknown atom type
 */
static int
handleAtom (struct HandlerEntry *handlers,
            const char *input,
            size_t size,
            size_t pos,
            struct ExtractContext *ec);

static struct HandlerEntry all_handlers[];

/**
 * Process atoms.
 * @return 0 on error, 1 for success, -1 for unknown atom type
 */
static int
processAtoms (struct HandlerEntry *handlers,
              const char *input,
              size_t size,
              struct ExtractContext *ec)
{
  size_t pos;

  if (size < sizeof (struct Atom))
    return 1;
  if (ec->depth >= MAX_ATOM_DEPTH)
    return 1;
  ec->depth++;
  pos = 0;
  while (pos < size - sizeof (struct Atom))
  {
    if (0 == handleAtom (handlers,
                         input,
                         size,
                         pos,
                         ec))
    {
      ec->depth--;
      return 0;
    }
    if (0 != ec->ret)
      break;                    /* processor asked us to stop */
    pos += getAtomSize (&input[pos]);
  }
  ec->depth--;
  return 1;
}


/**
 * Process all atoms.
 * @return 0 on error, 1 for success, -1 for unknown atom type
 */
static int
processAllAtoms (const char *input,
                 size_t size,
                 struct ExtractContext *ec)
{
  return processAtoms (all_handlers,
                       input,
                       size,
                       ec);
}


/**
 * Handle the moov atom.
 * @return 0 on error, 1 for success, -1 for unknown atom type
 */
static int
moovHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  uint32_t hdr = getAtomHeaderSize (&input[pos]);

  return processAllAtoms (&input[pos + hdr],
                          getAtomSize (&input[pos]) - hdr,
                          ec);
}


/* see http://developer.apple.com/documentation/QuickTime/QTFF/QTFFChap1/chapter_2_section_5.html */
struct FileType
{
  struct Atom header;
  /* major brand */
  char type[4];
  /* minor version */
  unsigned int version;
  /* compatible brands */
  char compatibility[4];
};


static int
ftypHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  const struct FileType *ft;

  if (getAtomSize (&input[pos]) < sizeof (struct FileType))
    return -1;
  ft = (const struct FileType *) &input[pos];

  for (unsigned i = 0;
       NULL != ftMap[i].ext;
       i++)
  {
    if (0 != memcmp (ft->type,
                     ftMap[i].ext,
                     4))
    {
      addKeyword (EXTRACTOR_METATYPE_MIMETYPE,
                  ftMap[i].mime,
                  ec);
      break;
    }
  }
  return 1;
}


/**
 * Handle the movie header ('mvhd') atom, reporting the movie duration.
 * Supports both the 32-bit (version 0) and 64-bit (version 1) layouts.
 *
 * @return 1 for success, -1 if the atom could not be parsed
 */
static int
mvhdHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  uint64_t asize = getAtomSize (&input[pos]);
  uint32_t hdr = getAtomHeaderSize (&input[pos]);
  const unsigned char *body;
  unsigned char version;
  uint64_t timeScale;
  uint64_t duration;
  char dur[24];

  if (asize < hdr + 4)
    return -1;
  body = (const unsigned char *) &input[pos + hdr];
  version = body[0];
  if (0 == version)
  {
    /* version(1) flags(3) creation(4) modification(4)
       timeScale(4) duration(4) ... */
    if (asize < hdr + 20)
      return -1;
    timeScale = ntohl (*(const uint32_t *) &body[12]);
    duration = ntohl (*(const uint32_t *) &body[16]);
  }
  else if (1 == version)
  {
    /* version(1) flags(3) creation(8) modification(8)
       timeScale(4) duration(8) ... */
    if (asize < hdr + 32)
      return -1;
    timeScale = ntohl (*(const uint32_t *) &body[20]);
    duration = ntohll (*(const uint64_t *) &body[24]);
  }
  else
  {
    return -1;
  }
  if (0 == timeScale)
    return -1;
  snprintf (dur,
            sizeof (dur),
            "%llus",
            (unsigned long long) (duration / timeScale));
  addKeyword (EXTRACTOR_METATYPE_DURATION,
              dur,
              ec);
  return 1;
}


struct CompressedMovieHeaderAtom
{
  struct Atom cmovAtom;
  struct Atom dcomAtom;
  char compressor[4];
  struct Atom cmvdAtom;
  uint32_t decompressedSize;
};


static int
cmovHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  const struct CompressedMovieHeaderAtom *c;
  unsigned int s;
  char *buf;
  int ret;
  z_stream z_state;
  int z_ret_code;

  if (getAtomSize (&input[pos]) < sizeof (struct CompressedMovieHeaderAtom))
    return -1;
  c = (const struct CompressedMovieHeaderAtom *) &input[pos];
  if ((ntohl (c->dcomAtom.size) != 12) ||
      (0 != memcmp (&c->dcomAtom.type, "dcom", 4)) ||
      (0 != memcmp (c->compressor, "zlib", 4)) ||
      (0 != memcmp (&c->cmvdAtom.type, "cmvd", 4)) ||
      (ntohl (c->cmvdAtom.size) !=
       getAtomSize (&input[pos]) - sizeof (struct Atom) * 2 - 4))
  {
    return -1;                  /* dcom must be 12 bytes */
  }
  s = ntohl (c->decompressedSize);
  if (s > MAX_CMOV_SIZE)
    return -1;                  /* ignore, too big! */
  buf = malloc (s);
  if (buf == NULL)
    return -1;                  /* out of memory, handle gracefully */

  memset (&z_state, 0, sizeof (z_state));
  z_state.next_in = (unsigned char *) &c[1];
  z_state.avail_in = ntohl (c->cmvdAtom.size);
  z_state.avail_out = s;
  z_state.next_out = (unsigned char *) buf;
  z_state.zalloc = (alloc_func) 0;
  z_state.zfree = (free_func) 0;
  z_state.opaque = (voidpf) 0;
  z_ret_code = inflateInit (&z_state);
  if (Z_OK != z_ret_code)
  {
    free (buf);
    return -1;                  /* crc error? */
  }
  z_ret_code = inflate (&z_state,
                        Z_NO_FLUSH);
  if ( (z_ret_code != Z_OK) &&
       (z_ret_code != Z_STREAM_END) )
  {
    inflateEnd (&z_state);
    free (buf);
    return -1;                  /* decode error? */
  }
  z_ret_code = inflateEnd (&z_state);
  if (Z_OK != z_ret_code)
  {
    free (buf);
    return -1;                  /* decode error? */
  }
  ret = handleAtom (all_handlers,
                    buf,
                    s,
                    0,
                    ec);
  free (buf);
  return ret;
}


/**
 * Handle the track header ('tkhd') atom.  The (fixed-point) track
 * width and height are the final eight bytes of the atom regardless of
 * the atom's version, so we read them relative to the end of the box.
 *
 * @return 1 for success, -1 if the atom could not be parsed
 */
static int
tkhdHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  uint64_t asize = getAtomSize (&input[pos]);
  uint32_t hdr = getAtomHeaderSize (&input[pos]);
  const unsigned char *p;
  unsigned int width;
  unsigned int height;
  char dimensions[40];

  if (asize < hdr + 8)
    return -1;
  p = (const unsigned char *) &input[pos + asize - 8];
  /* 16.16 fixed point; the integer part is the high 16 bits */
  width = (p[0] << 8) | p[1];
  height = (p[4] << 8) | p[5];
  if (0 != width)
  {
    /* if actually a/the video track */
    snprintf (dimensions,
              sizeof (dimensions),
              "%ux%u",
              width,
              height);
    addKeyword (EXTRACTOR_METATYPE_IMAGE_DIMENSIONS,
                dimensions,
                ec);
  }
  return 1;
}


static int
trakHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  uint32_t hdr = getAtomHeaderSize (&input[pos]);

  return processAllAtoms (&input[pos + hdr],
                          getAtomSize (&input[pos]) - hdr,
                          ec);
}


static int
metaHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  uint32_t hdr = getAtomHeaderSize (&input[pos]);

  if (getAtomSize (&input[pos]) < hdr + 4)
    return -1;
  return processAllAtoms (&input[pos + hdr + 4],
                          getAtomSize (&input[pos]) - hdr - 4,
                          ec);
}


struct InternationalText
{
  struct Atom header;
  uint16_t length;
  uint16_t language;
};


/*
 * see http://developer.apple.com/documentation/QuickTime/QTFF/QTFFChap2/chapter_3_section_2.html
 *   "User Data Text Strings and Language Codes"
 */
static int
processTextTag (const char *input,
                size_t size,
                size_t pos,
                enum EXTRACTOR_MetaType type, struct ExtractContext *ec)
{
  uint64_t as;
  uint16_t len;
  uint16_t lang;
  const struct InternationalText *txt;
  char *meta;

  /* contains "international text":
     16-bit size + 16 bit language code */
  as = getAtomSize (&input[pos]);
  if (as < sizeof (struct InternationalText))
    return -1;                  /* invalid */
  txt = (const struct InternationalText *) &input[pos];
  len = ntohs (txt->length);
  if (len + sizeof (struct InternationalText) > as)
    return -1;                  /* invalid */
  lang = ntohs (txt->language);
  if (lang < sizeof (languages) / sizeof (char *))
    addKeyword (EXTRACTOR_METATYPE_LANGUAGE,
                languages[lang],
                ec);

  meta = malloc (len + 1);
  if (NULL == meta)
    return -1;
  memcpy (meta,
          &txt[1],
          len);
  meta[len] = '\0';
  for (unsigned int i = 0; i < len; i++)
    if (meta[i] == '\r')
      meta[i] = '\n';
  addKeyword (type,
              meta,
              ec);
  free (meta);
  return 1;
}


static int
c_Handler (const char *input,
           size_t size,
           size_t pos,
           struct ExtractContext *ec)
{
  for (unsigned int i = 0;
       NULL != cHm[i].pfx;
       i++)
    if (0 == memcmp (&input[pos + 5],
                     cHm[i].pfx,
                     3))
      return processTextTag (input,
                             size,
                             pos,
                             cHm[i].type,
                             ec);
  return -1;  /* not found */
}


/**
 * Process the 'data' atom nested inside an iTunes-style 'ilst' entry.
 *
 * @param input start of the buffer
 * @param size size of the parent (ilst entry) atom
 * @param pos offset of the 'data' atom within @a input
 * @param patom pointer to the parent (ilst entry) atom
 * @param type metadata type to report the value as
 * @return 1 for success, -1 if the atom could not be handled
 */
static int
processDataAtom (const char *input,
                 size_t size, /* parent atom size */
                 size_t pos,
                 const char *patom,
                 enum EXTRACTOR_MetaType type,
                 struct ExtractContext *ec)
{
  char *meta;
  unsigned char version;
  unsigned int wellknown;
  uint64_t asize;
  unsigned int len;
  uint32_t hdr;
  int i;

  hdr = getAtomHeaderSize (&input[pos]);
  asize = getAtomSize (&input[pos]);
  if (0 !=
      memcmp (&input[pos + 4],
              "data",
              4))
    return -1;

  if ((asize < hdr + 8) ||      /* header + u32 type + u32 locale */
      (asize > (getAtomSize (&patom[0]) - 8)))
    return -1;

  len = (unsigned int) (asize - (hdr + 8));

  version = input[pos + 8];
  /* "well known type" indicator (the low 24 bits of the type field) */
  wellknown = ((unsigned char) input[pos + 9] << 16)
              | ((unsigned char) input[pos + 10] << 8)
              | (unsigned char) input[pos + 11];

  if (0 != version)
    return -1;

  /* cover art: well-known type 13 = JPEG, 14 = PNG, 27 = BMP */
  if ( (EXTRACTOR_METATYPE_COVER_PICTURE == type) &&
       ( (13 == wellknown) ||
         (14 == wellknown) ||
         (27 == wellknown) ) )
  {
    const char *mime;

    if (0 == len)
      return -1;
    switch (wellknown)
    {
    case 13:
      mime = "image/jpeg";
      break;
    case 14:
      mime = "image/png";
      break;
    default:
      mime = "image/bmp";
      break;
    }
    addBinary (type,
               mime,
               &input[pos + 16],
               len,
               ec);
    return 1;
  }

  if (0x0 == wellknown)         /* binary data */
  {
    if (0 ==
        memcmp (&patom[4],
                "gnre",
                4))
    {
      if (len >= 2)
      {
        uint16_t genre = ((uint8_t) input[pos + 16] << 8)
                         | (uint8_t) input[pos + 17];

        if ((genre > 0) && (genre <= GENRE_NAME_COUNT))
          addKeyword (type,
                      genre_names[genre - 1],
                      ec);
      }
      return 1;
    }
    else if ( (0 ==
               memcmp (&patom[4],
                       "trkn",
                       4)) ||
              (0 ==
               memcmp (&patom[4],
                       "disk",
                       4)))
    {
      if (len >= 4)
      {
        unsigned short n = ((unsigned char) input[pos + 18] << 8)
                           | (unsigned char) input[pos + 19];
        char s[8];

        snprintf (s,
                  sizeof (s),
                  "%d",
                  n);
        addKeyword (type,
                    s,
                    ec);
      }
      return 1;
    }
    else if (0 ==
             memcmp (&patom[4],
                     "tmpo",
                     4))
    {
      if (len >= 2)
      {
        unsigned short n = ((unsigned char) input[pos + 16] << 8)
                           | (unsigned char) input[pos + 17];
        char s[8];

        snprintf (s,
                  sizeof (s),
                  "%u",
                  n);
        addKeyword (type,
                    s,
                    ec);
      }
      return 1;
    }
    else
    {
      return -1;
    }
  }
  else if (0x15 == wellknown)   /* signed/unsigned big-endian integer */
  {
    unsigned long long n = 0;
    char s[24];
    unsigned int j;

    if ((len < 1) || (len > 8))
      return -1;
    for (j = 0; j < len; j++)
      n = (n << 8) | (unsigned char) input[pos + 16 + j];
    snprintf (s,
              sizeof (s),
              "%llu",
              n);
    addKeyword (type,
                s,
                ec);
    return 1;
  }
  else if (wellknown == 0x1)    /* UTF-8 text data */
  {
    meta = malloc (len + 1);
    if (meta == NULL)
      return -1;
    memcpy (meta,
            &input[pos + 16],
            len);
    meta[len] = '\0';
    for (i = 0; i < len; i++)
      if (meta[i] == '\r')
        meta[i] = '\n';
    addKeyword (type,
                meta,
                ec);
    free (meta);
    return 1;
  }

  return -1;
}


/* NOTE: iTunes tag processing should, in theory, be limited to iTunes
 * file types (from ftyp), but, in reality, it seems that there are other
 * files, like 3gpp, out in the wild with iTunes tags. */
static int
iTunesTagHandler (const char *input,
                  size_t size,
                  size_t pos,
                  struct ExtractContext *ec)
{
  uint64_t asize;
  uint32_t hdr;

  hdr = getAtomHeaderSize (&input[pos]);
  asize = getAtomSize (&input[pos]);

  if (asize < hdr + 8)          /* header + at least one atom */
    return -1;

  for (unsigned int i = 0;
       NULL != it_to_extr_table[i].atom_type;
       i++)
    if (0 == memcmp (&input[pos + 4],
                     it_to_extr_table[i].atom_type,
                     4))
      return processDataAtom (input,
                              asize,
                              pos + hdr,
                              &input[pos],
                              it_to_extr_table[i].type,
                              ec);
  return -1;
}


/**
 * Handle the iTunes metadata list ('ilst').  Its children have
 * arbitrary four-character keys, so rather than a name table we simply
 * iterate them and let #iTunesTagHandler decide what is interesting.
 *
 * @return 0 on a fatal error, 1 otherwise
 */
static int
ilstHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  uint32_t hdr = getAtomHeaderSize (&input[pos]);
  size_t end = pos + getAtomSize (&input[pos]);
  size_t cpos = pos + hdr;

  if (ec->depth >= MAX_ATOM_DEPTH)
    return 1;
  ec->depth++;
  while ((cpos + sizeof (struct Atom) <= end) &&
         (checkAtomValid (input, end, cpos)))
  {
    iTunesTagHandler (input, end, cpos, ec);
    if (0 != ec->ret)
      break;
    cpos += getAtomSize (&input[cpos]);
  }
  ec->depth--;
  return 1;
}


/**
 * Handle the user-data ('udta') atom.  It mixes classic QuickTime
 * '(C)xyz' international-text tags with structural sub-atoms such as
 * 'meta'/'ilst', so we iterate the children and dispatch accordingly.
 *
 * @return 0 on a fatal error, 1 otherwise
 */
static int
udtaHandler (const char *input,
             size_t size,
             size_t pos,
             struct ExtractContext *ec)
{
  uint32_t hdr = getAtomHeaderSize (&input[pos]);
  size_t end = pos + getAtomSize (&input[pos]);
  size_t cpos = pos + hdr;

  if (ec->depth >= MAX_ATOM_DEPTH)
    return 1;
  ec->depth++;
  while ((cpos + sizeof (struct Atom) <= end) &&
         (checkAtomValid (input, end, cpos)))
  {
    if (0xA9 == (unsigned char) input[cpos + 4])
      c_Handler (input,
                 end,
                 cpos,
                 ec);
    else
      handleAtom (all_handlers,
                  input,
                  end,
                  cpos,
                  ec);
    if (0 != ec->ret)
      break;
    cpos += getAtomSize (&input[cpos]);
  }
  ec->depth--;
  return 1;
}


static struct HandlerEntry all_handlers[] = {
  {"moov", &moovHandler},
  {"cmov", &cmovHandler},
  {"mvhd", &mvhdHandler},
  {"trak", &trakHandler},
  {"tkhd", &tkhdHandler},
  {"ilst", &ilstHandler},
  {"meta", &metaHandler},
  {"udta", &udtaHandler},
  {"ftyp", &ftypHandler},
  {NULL, NULL},
};


/**
 * Call the handler for the atom at the given position.
 * @return 0 on error, 1 for success, -1 for unknown atom type
 */
static int
handleAtom (struct HandlerEntry *handlers,
            const char *input,
            size_t size,
            size_t pos,
            struct ExtractContext *ec)
{
  if (! checkAtomValid (input,
                        size,
                        pos))
    return 0;
  for (unsigned i = 0;
       handlers[i].name != NULL;
       i++)
  {
    if (0 ==
        memcmp (&input[pos + 4],
                handlers[i].name,
                4))
    {
      return handlers[i].handler (input,
                                  size,
                                  pos,
                                  ec);
    }
  }
  return -1;
}


/**
 * Read exactly @a len bytes from absolute offset @a off into @a dst.
 *
 * The extraction context exposes the file through a sliding shared
 * memory window, so a single read may return fewer bytes than
 * requested; we seek once and then loop until the request is satisfied.
 *
 * @return 0 on success, -1 on error / short file
 */
static int
qt_pread (struct EXTRACTOR_ExtractContext *ec,
          uint64_t off,
          void *dst,
          size_t len)
{
  unsigned char *out = dst;

  if ((int64_t) off != ec->seek (ec->cls, (int64_t) off, SEEK_SET))
    return -1;
  while (len > 0)
  {
    void *buf;
    ssize_t got;

    got = ec->read (ec->cls,
                    &buf,
                    len);
    if (got <= 0)
      return -1;
    memcpy (out,
            buf,
            (size_t) got);
    out += got;
    len -= (size_t) got;
  }
  return 0;
}


/**
 * Top-level atom types worth pulling into memory.  Everything else
 * (notably the huge 'mdat' payload, plus 'free'/'skip'/'wide') is
 * skipped without ever being read.
 */
static bool
is_interesting_top_atom (const unsigned char *type)
{
  static const char *const interesting[] = {
    "moov", "ftyp", "meta", "udta", "uuid", "pnot", NULL
  };

  for (unsigned int i = 0;
       NULL != interesting[i];
       i++)
    if (0 == memcmp (type,
                     interesting[i],
                     4))
      return true;
  return false;
}


/**
 * Main entry method for the QuickTime/MP4 extraction plugin.
 *
 * @param ec extraction context provided to the plugin
 */
void
EXTRACTOR_qt_extract_method (struct EXTRACTOR_ExtractContext *ec);

void
EXTRACTOR_qt_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  struct ExtractContext xc;
  uint64_t fsize;
  uint64_t pos;

  fsize = ec->get_size (ec->cls);
  if ((UINT64_MAX == fsize) || (fsize < sizeof (struct Atom)))
    return;

  xc.proc = ec->proc;
  xc.proc_cls = ec->cls;
  xc.ret = 0;
  xc.depth = 0;

  pos = 0;
  while ( (0 == xc.ret) &&
          (pos + sizeof (struct Atom) <= fsize) )
  {
    unsigned char hdr[16];
    uint64_t asize;
    unsigned int hsize;

    if (0 != qt_pread (ec, pos, hdr, 8))
      break;
    asize = ((uint64_t) hdr[0] << 24) | ((uint64_t) hdr[1] << 16)
            | ((uint64_t) hdr[2] << 8) | (uint64_t) hdr[3];
    if (1 == asize)
    {
      if ((pos + 16 > fsize) ||
          (0 != qt_pread (ec, pos + 8, &hdr[8], 8)))
        break;
      asize = ((uint64_t) hdr[8] << 56) | ((uint64_t) hdr[9] << 48)
              | ((uint64_t) hdr[10] << 40) | ((uint64_t) hdr[11] << 32)
              | ((uint64_t) hdr[12] << 24) | ((uint64_t) hdr[13] << 16)
              | ((uint64_t) hdr[14] << 8) | (uint64_t) hdr[15];
      hsize = 16;
    }
    else if (0 == asize)
    {
      /* atom extends to end of file */
      asize = fsize - pos;
      hsize = 8;
    }
    else
    {
      hsize = 8;
    }
    if ((asize < hsize) || (pos + asize > fsize))
      break;

    if (is_interesting_top_atom (&hdr[4]) &&
        (asize <= MAX_ATOM_SIZE))
    {
      char *buf = malloc ((size_t) asize);

      if (NULL != buf)
      {
        if (0 == qt_pread (ec,
                           pos,
                           buf,
                           (size_t) asize))
          handleAtom (all_handlers,
                      buf,
                      (size_t) asize,
                      0,
                      &xc);
        free (buf);
      }
    }
    pos += asize;
  }
}


/*  end of qt_extractor.c */
