/*
 * Copyright 2016 CSIRO
 *
 * This file is part of Mastik.
 *
 * Mastik is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mastik is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mastik.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __L1_H__
#define __L1_H__ 1

#define L1_SETS 64

typedef struct l1pp *l1pp_t;

l1pp_t l1_prepare(void);
void l1_release(l1pp_t l1);

int l1_monitor(l1pp_t l1, int line);
int l1_unmonitor(l1pp_t l1, int line);
void l1_monitorall(l1pp_t l1);
void l1_unmonitorall(l1pp_t l1);
int l1_getmonitoredset(l1pp_t l1, int *lines, int nlines);
void l1_randomise(l1pp_t l1);

void l1_probe(l1pp_t l1, uint16_t *results);
void l1_bprobe(l1pp_t l1, uint16_t *results);

// Slot is currently not implemented
int l1_repeatedprobe(l1pp_t l1, int nrecords, uint16_t *results, int slot);



#endif // __L1_H__

