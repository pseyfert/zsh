/*
 * Copyright (C) 2019 CERN for the benefit of the LHCb collaboration
 * Author: Paul Seyfert <pseyfert@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In applying this licence, CERN does not waive the privileges and immunities
 * granted to it by virtue of its status as an Intergovernmental Organization
 * or submit itself to any jurisdiction.
 */

#include <stdlib.h>    // malloc
#include <string.h>    // strlen
#include <sys/param.h> // MIN

static const char *on = "%F{red}%B";
static const char *off = "%b%f";
// static const char *on = "\033[1m\033[31m";
// static const char *off = "\033[0m";

// main function
int high(char *inA, char *inB, char **outA, char **outB);
// count the percent signs in a string
size_t percents(char *c);
// determine the maximum number of toggles between highlight and regular font face
// purely based on their lengths
size_t maxtoggle(const char *a, const char *b);
// (see below)
size_t equalend(const char *one, const char *two);
size_t equalbeg(const char *one, const char *two);
// allocate new string and escape all percent signs (for zsh prompts)
char *escape(char *in, size_t p);

size_t percents(char *c) {
  size_t count = 0;
  for (size_t i = 0; i < strlen(c); ++i) {
    if (c[i] == '%')
      count++;
  }
  return count;
}

char *escape(char *in, size_t p) {
  char *o = (char *)malloc(1 + strlen(in) + p);
  size_t ii = 0;
  for (size_t i = 0; i < strlen(in); ++i) {
    o[ii++] = in[i];
    if (in[i] == '%') {
      o[ii++] = '%';
    }
    // if (ii - i == p)
    //   break;
  }
  for ( ; ii < 1 + strlen(in) + p ; ++ii) {
    o[ii] = '\0';
  }
  return o;
}

// returns the index of the first character that's different
size_t equalbeg(const char *one, const char *two) {
  size_t shorter = MIN(strlen(one), strlen(two));
  for (size_t i = 0; i < shorter; i++) {
    if (one[i] != two[i]) {
      return i;
    }
  }
  return shorter;
}

// returns the quasi-index of the first character that's different
// i.e. 0 if the last char differs
size_t equalend(const char *one, const char *two) {
  size_t o = strlen(one);
  size_t t = strlen(two);
  size_t shorter = MIN(o, t) - equalbeg(one, two);

  for (size_t i = 0; i < shorter; i++) {
    if (one[o - i - 1] != two[t - i - 1]) {
      return i;
    }
  }
  return shorter;
}

size_t maxtoggle(const char *a, const char *b) {
  size_t relevantlength = MAX(strlen(a), strlen(b));
  // 1 char -> max 1 toggle
  // 2 char -> max 1 toggle
  // 3 char -> max 2 toggle
  // 4 char -> max 2 toggle
  // 5 char -> max 3 toggle
  return (1 + relevantlength) / 2;
}

int high(char *inA, char *inB, char **outA, char **outB) {
  size_t percentsA = percents(inA);
  size_t percentsB = percents(inB);

  size_t expected_A_length = (1 + strlen(inA) + percentsA +
                         maxtoggle(inA, inB) * (strlen(on) + strlen(off)));
  size_t expected_B_length = (1 + strlen(inB) + percentsB +
                         maxtoggle(inA, inB) * (strlen(on) + strlen(off)));
  // NB: escaping percent signs don't add toggles, but add characters
  *outA = (char *)malloc(expected_A_length);
  *outB = (char *)malloc(expected_B_length);

  if (percentsA > 0)
    inA = escape(inA, percentsA);
  if (percentsB > 0)
    inB = escape(inB, percentsB);

  size_t goodstart = equalbeg(inA, inB);

  for (size_t i = 0; i < goodstart; ++i) {
    (*outA)[i] = inA[i];
    (*outB)[i] = inB[i];
  }

  size_t iA = goodstart;
  size_t iB = goodstart;
  size_t oA = goodstart;
  size_t oB = goodstart;

  size_t trailer = equalend(inA, inB);
  char* cutoffA;
  char* cutoffB;
  if (trailer > 0) {
    size_t trailerA = MIN(trailer, strlen(inA) - goodstart);
    size_t trailerB = MIN(trailer, strlen(inB) - goodstart);
    size_t len_frontA = strlen(inA) - trailerA;
    size_t len_frontB = strlen(inB) - trailerB;
    cutoffA = (char *)malloc(1+len_frontA);
    cutoffB = (char *)malloc(1+len_frontB);
    strncpy(cutoffA, inA, len_frontA);
    strncpy(cutoffB, inB, len_frontB);
    cutoffA[len_frontA] = 0;
    cutoffB[len_frontB] = 0;
  } else {
    cutoffA = inA;
    cutoffB = inB;
  }

  int redA = 0;
  // int redB = 0; // redundant ATM

  for (; iA < strlen(cutoffA) && iB < strlen(cutoffB);) {
    if (cutoffA[iA] == cutoffB[iB]) {
      if (redA) {
        for (size_t tmp = 0; tmp < strlen(off); ++tmp) {
          (*outA)[oA++] = off[tmp];
          (*outB)[oB++] = off[tmp];
        }
        redA = !redA;
      }
      (*outA)[oA++] = cutoffA[iA++];
      (*outB)[oB++] = cutoffB[iB++];
    } else {
      if (!redA) {
        for (size_t tmp = 0; tmp < strlen(on); ++tmp) {
          (*outA)[oA++] = on[tmp];
          (*outB)[oB++] = on[tmp];
        }
        redA = !redA;
      }
      if (cutoffA[iA] != '%')
        (*outA)[oA++] = cutoffA[iA++];
      else {
        (*outA)[oA++] = cutoffA[iA++];
        (*outA)[oA++] = cutoffA[iA++];
      }
      if (cutoffB[iB] != '%')
        (*outB)[oB++] = cutoffB[iB++];
      else {
        (*outB)[oB++] = cutoffB[iB++];
        (*outB)[oB++] = cutoffB[iB++];
      }
    }
  }
  if (iA < strlen(cutoffA)) {
    if (!redA) {
      for (size_t tmp = 0; tmp < strlen(on); ++tmp) {
        (*outA)[oA++] = on[tmp];
      }
    }
    for (; iA < strlen(cutoffA);) {
      (*outA)[oA++] = cutoffA[iA++];
    }
    for (size_t tmp = 0; tmp < strlen(off); ++tmp) {
      (*outA)[oA++] = off[tmp];
    }
  } else {
    if (redA) {
      for (size_t tmp = 0; tmp < strlen(off); ++tmp) {
        (*outA)[oA++] = off[tmp];
      }
    }
  }
  if (iB < strlen(cutoffB)) {
    if (!redA) {
      for (size_t tmp = 0; tmp < strlen(on); ++tmp) {
        (*outB)[oB++] = on[tmp];
      }
    }
    for (; iB < strlen(cutoffB);) {
      (*outB)[oB++] = cutoffB[iB++];
    }
    for (size_t tmp = 0; tmp < strlen(off); ++tmp) {
      (*outB)[oB++] = off[tmp];
    }
  } else {
    if (redA) {
      for (size_t tmp = 0; tmp < strlen(off); ++tmp) {
        (*outB)[oB++] = off[tmp];
      }
    }
  }

  for (size_t tmp = strlen(cutoffA) ; tmp < strlen(inA); ++tmp) {
    (*outA)[oA++] = inA[tmp];
    (*outB)[oB++] = inA[tmp];
  }

  (*outA)[oA] = '\0';
  (*outB)[oB] = '\0';

  if (trailer > 0) {
    free(cutoffA);
    free(cutoffB);
  }
  if (percentsA > 0)
    free(inA);
  if (percentsB > 0)
    free(inB);
  return 0;
}
