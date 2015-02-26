/*
 * autogenerated by src/backend/utils/sort/gen_qsort_tuple.pl, do not edit
 * This file is included by tuplesort.c, rather than compiled separately.
 */

/*	$NetBSD: qsort.c,v 1.13 2003/08/07 16:43:42 agc Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Qsort routine based on J. L. Bentley and M. D. McIlroy,
 * "Engineering a sort function",
 * Software--Practice and Experience 23 (1993) 1249-1265.
 * We have modified their original by adding a check for already-sorted input,
 * which seems to be a win per discussions on pgsql-hackers around 2006-03-21.
 */

static void
swapfunc(SortTuple *a, SortTuple *b, size_t n)
{
	do
	{
		SortTuple 	t = *a;
		*a++ = *b;
		*b++ = t;
	} while (--n > 0);
}

#define swap(a, b)						\
	do { 								\
		SortTuple t = *(a);				\
		*(a) = *(b);					\
		*(b) = t;						\
	} while (0);

#define vecswap(a, b, n) if ((n) > 0) swapfunc((a), (b), (size_t)(n))
static SortTuple *
med3_tuple(SortTuple *a, SortTuple *b, SortTuple *c, SortTupleComparator cmp_tuple, Tuplesortstate *state)
{
	return cmp_tuple(a, b, state) < 0 ?
		(cmp_tuple(b, c, state) < 0 ? b :
			(cmp_tuple(a, c, state) < 0 ? c : a))
		: (cmp_tuple(b, c, state) > 0 ? b :
			(cmp_tuple(a, c, state) < 0 ? a : c));
}

static void
qsort_tuple(SortTuple *a, size_t n, SortTupleComparator cmp_tuple, Tuplesortstate *state)
{
	SortTuple  *pa,
			   *pb,
			   *pc,
			   *pd,
			   *pl,
			   *pm,
			   *pn;
	int			d,
				r,
				presorted;

loop:
	CHECK_FOR_INTERRUPTS();
	if (n < 7)
	{
		for (pm = a + 1; pm < a + n; pm++)
			for (pl = pm; pl > a && cmp_tuple(pl - 1, pl, state) > 0; pl--)
				swap(pl, pl - 1);
		return;
	}
	presorted = 1;
	for (pm = a + 1; pm < a + n; pm++)
	{
		CHECK_FOR_INTERRUPTS();
		if (cmp_tuple(pm - 1, pm, state) > 0)
		{
			presorted = 0;
			break;
		}
	}
	if (presorted)
		return;
	pm = a + (n / 2);
	if (n > 7)
	{
		pl = a;
		pn = a + (n - 1);
		if (n > 40)
		{
			d = (n / 8);
			pl = med3_tuple(pl, pl + d, pl + 2 * d, cmp_tuple, state);
			pm = med3_tuple(pm - d, pm, pm + d, cmp_tuple, state);
			pn = med3_tuple(pn - 2 * d, pn - d, pn, cmp_tuple, state);
		}
		pm = med3_tuple(pl, pm, pn, cmp_tuple, state);
	}
	swap(a, pm);
	pa = pb = a + 1;
	pc = pd = a + (n - 1);
	for (;;)
	{
		while (pb <= pc && (r = cmp_tuple(pb, a, state)) <= 0)
		{
			CHECK_FOR_INTERRUPTS();
			if (r == 0)
			{
				swap(pa, pb);
				pa++;
			}
			pb++;
		}
		while (pb <= pc && (r = cmp_tuple(pc, a, state)) >= 0)
		{
			CHECK_FOR_INTERRUPTS();
			if (r == 0)
			{
				swap(pc, pd);
				pd--;
			}
			pc--;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		pb++;
		pc--;
	}
	pn = a + n;
	r = Min(pa - a, pb - pa);
	vecswap(a, pb - r, r);
	r = Min(pd - pc, pn - pd - 1);
	vecswap(pb, pn - r, r);
	if ((r = pb - pa) > 1)
		qsort_tuple(a, r, cmp_tuple, state);
	if ((r = pd - pc) > 1)
	{
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r;
		goto loop;
	}
/*		qsort_tuple(pn - r, r, cmp_tuple, state);*/
}

#define cmp_ssup(a, b, ssup) \
	ApplySortComparator((a)->datum1, (a)->isnull1, \
						(b)->datum1, (b)->isnull1, ssup)
static SortTuple *
med3_ssup(SortTuple *a, SortTuple *b, SortTuple *c, SortSupport ssup)
{
	return cmp_ssup(a, b, ssup) < 0 ?
		(cmp_ssup(b, c, ssup) < 0 ? b :
			(cmp_ssup(a, c, ssup) < 0 ? c : a))
		: (cmp_ssup(b, c, ssup) > 0 ? b :
			(cmp_ssup(a, c, ssup) < 0 ? a : c));
}

static void
qsort_ssup(SortTuple *a, size_t n, SortSupport ssup)
{
	SortTuple  *pa,
			   *pb,
			   *pc,
			   *pd,
			   *pl,
			   *pm,
			   *pn;
	int			d,
				r,
				presorted;

loop:
	CHECK_FOR_INTERRUPTS();
	if (n < 7)
	{
		for (pm = a + 1; pm < a + n; pm++)
			for (pl = pm; pl > a && cmp_ssup(pl - 1, pl, ssup) > 0; pl--)
				swap(pl, pl - 1);
		return;
	}
	presorted = 1;
	for (pm = a + 1; pm < a + n; pm++)
	{
		CHECK_FOR_INTERRUPTS();
		if (cmp_ssup(pm - 1, pm, ssup) > 0)
		{
			presorted = 0;
			break;
		}
	}
	if (presorted)
		return;
	pm = a + (n / 2);
	if (n > 7)
	{
		pl = a;
		pn = a + (n - 1);
		if (n > 40)
		{
			d = (n / 8);
			pl = med3_ssup(pl, pl + d, pl + 2 * d, ssup);
			pm = med3_ssup(pm - d, pm, pm + d, ssup);
			pn = med3_ssup(pn - 2 * d, pn - d, pn, ssup);
		}
		pm = med3_ssup(pl, pm, pn, ssup);
	}
	swap(a, pm);
	pa = pb = a + 1;
	pc = pd = a + (n - 1);
	for (;;)
	{
		while (pb <= pc && (r = cmp_ssup(pb, a, ssup)) <= 0)
		{
			CHECK_FOR_INTERRUPTS();
			if (r == 0)
			{
				swap(pa, pb);
				pa++;
			}
			pb++;
		}
		while (pb <= pc && (r = cmp_ssup(pc, a, ssup)) >= 0)
		{
			CHECK_FOR_INTERRUPTS();
			if (r == 0)
			{
				swap(pc, pd);
				pd--;
			}
			pc--;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		pb++;
		pc--;
	}
	pn = a + n;
	r = Min(pa - a, pb - pa);
	vecswap(a, pb - r, r);
	r = Min(pd - pc, pn - pd - 1);
	vecswap(pb, pn - r, r);
	if ((r = pb - pa) > 1)
		qsort_ssup(a, r, ssup);
	if ((r = pd - pc) > 1)
	{
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r;
		goto loop;
	}
/*		qsort_ssup(pn - r, r, ssup);*/
}

