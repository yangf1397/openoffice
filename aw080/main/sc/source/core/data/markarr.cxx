/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"



// INCLUDE ---------------------------------------------------------------

#include <tools/debug.hxx>
#include "markarr.hxx"
#include "global.hxx"
#include "address.hxx"
#include <memory.h>

// STATIC DATA -----------------------------------------------------------

//------------------------------------------------------------------------

ScMarkArray::ScMarkArray() :
    nCount( 0 ),
    nLimit( 0 ),
    pData( NULL )
{
    // special case "no marks" with pData = NULL
}

//------------------------------------------------------------------------

ScMarkArray::~ScMarkArray()
{
    delete[] pData;
}

//------------------------------------------------------------------------

void ScMarkArray::Reset( sal_Bool bMarked )
{
    // always create pData here
    // (or have separate method to ensure pData)

    delete[] pData;

    nCount = nLimit = 1;
    pData = new ScMarkEntry[1];
    pData[0].nRow = MAXROW;
    pData[0].bMarked = bMarked;
}

//------------------------------------------------------------------------

sal_Bool ScMarkArray::Search( SCROW nRow, SCSIZE& nIndex ) const
{
	long	nLo 		= 0;
	long	nHi 		= static_cast<long>(nCount) - 1;
	long	nStartRow	= 0;
	long	nEndRow 	= 0;
	long	i			= 0;
	sal_Bool	bFound		= (nCount == 1);
	if (pData)
	{
		while ( !bFound && nLo <= nHi )
		{
			i = (nLo + nHi) / 2;
			if (i > 0)
				nStartRow = (long) pData[i - 1].nRow;
			else
				nStartRow = -1;
			nEndRow = (long) pData[i].nRow;
			if (nEndRow < (long) nRow)
				nLo = ++i;
			else
				if (nStartRow >= (long) nRow)
					nHi = --i;
				else
					bFound = sal_True;
		}
	}
	else
		bFound = sal_False;

	if (bFound)
		nIndex=(SCSIZE)i;
	else
		nIndex=0;
	return bFound;
}

sal_Bool ScMarkArray::GetMark( SCROW nRow ) const
{
	SCSIZE i;
	if (Search( nRow, i ))
		return pData[i].bMarked;
	else
		return sal_False;

}

//------------------------------------------------------------------------

void ScMarkArray::SetMarkArea( SCROW nStartRow, SCROW nEndRow, sal_Bool bMarked )
{
	if (ValidRow(nStartRow) && ValidRow(nEndRow))
	{
		if ((nStartRow == 0) && (nEndRow == MAXROW))
		{
			Reset(bMarked);
		}
		else
		{
            if (!pData)
                Reset(sal_False);   // create pData for further processing - could use special case handling!

            SCSIZE nNeeded = nCount + 2;
            if ( nLimit < nNeeded )
            {
                nLimit += SC_MARKARRAY_DELTA;
                if ( nLimit < nNeeded )
                    nLimit = nNeeded;
                ScMarkEntry* pNewData = new ScMarkEntry[nLimit];
                memcpy( pNewData, pData, nCount*sizeof(ScMarkEntry) );
				delete[] pData;
				pData = pNewData;
            }

            SCSIZE ni;          // number of entries in beginning
            SCSIZE nInsert;     // insert position (MAXROW+1 := no insert)
            sal_Bool bCombined = sal_False;
            sal_Bool bSplit = sal_False;
            if ( nStartRow > 0 )
            {
                // skip beginning
                SCSIZE nIndex;
                Search( nStartRow, nIndex );
                ni = nIndex;

                nInsert = MAXROWCOUNT;
                if ( pData[ni].bMarked != bMarked )
                {
                    if ( ni == 0 || (pData[ni-1].nRow < nStartRow - 1) )
                    {   // may be a split or a simple insert or just a shrink,
                        // row adjustment is done further down
                        if ( pData[ni].nRow > nEndRow )
                            bSplit = sal_True;
                        ni++;
                        nInsert = ni;
                    }
                    else if ( ni > 0 && pData[ni-1].nRow == nStartRow - 1 )
                        nInsert = ni;
                }
                if ( ni > 0 && pData[ni-1].bMarked == bMarked )
                {   // combine
                    pData[ni-1].nRow = nEndRow;
                    nInsert = MAXROWCOUNT;
                    bCombined = sal_True;
                }
            }
            else
	    {
                nInsert = 0;
                ni = 0;
	    }

            SCSIZE nj = ni;     // stop position of range to replace
            while ( nj < nCount && pData[nj].nRow <= nEndRow )
                nj++;
            if ( !bSplit )
            {
                if ( nj < nCount && pData[nj].bMarked == bMarked )
                {   // combine
                    if ( ni > 0 )
                    {
                        if ( pData[ni-1].bMarked == bMarked )
                        {   // adjacent entries
                            pData[ni-1].nRow = pData[nj].nRow;
                            nj++;
                        }
                        else if ( ni == nInsert )
                            pData[ni-1].nRow = nStartRow - 1;   // shrink
                    }
                    nInsert = MAXROWCOUNT;
                    bCombined = sal_True;
                }
                else if ( ni > 0 && ni == nInsert )
                    pData[ni-1].nRow = nStartRow - 1;   // shrink
            }
            if ( ni < nj )
            {   // remove middle entries
                if ( !bCombined )
                {   // replace one entry
                    pData[ni].nRow = nEndRow;
                    pData[ni].bMarked = bMarked;
                    ni++;
                    nInsert = MAXROWCOUNT;
                }
                if ( ni < nj )
                {   // remove entries
                    memmove( pData + ni, pData + nj, (nCount - nj) * sizeof(ScMarkEntry) );
                    nCount -= nj - ni;
                }
            }

            if ( nInsert < sal::static_int_cast<SCSIZE>(MAXROWCOUNT) )
            {   // insert or append new entry
                if ( nInsert <= nCount )
                {
                    if ( !bSplit )
                        memmove( pData + nInsert + 1, pData + nInsert,
                            (nCount - nInsert) * sizeof(ScMarkEntry) );
                    else
                    {
                        memmove( pData + nInsert + 2, pData + nInsert,
                            (nCount - nInsert) * sizeof(ScMarkEntry) );
                        pData[nInsert+1] = pData[nInsert-1];
                        nCount++;
                    }
                }
                if ( nInsert )
                    pData[nInsert-1].nRow = nStartRow - 1;
                pData[nInsert].nRow = nEndRow;
                pData[nInsert].bMarked = bMarked;
                nCount++;
            }
		}
	}
//	InfoBox(0, String(nCount) + String(" Eintraege") ).Execute();
}

//UNUSED2009-05 void ScMarkArray::DeleteArea(SCROW nStartRow, SCROW nEndRow)
//UNUSED2009-05 {
//UNUSED2009-05     SetMarkArea(nStartRow, nEndRow, sal_False);
//UNUSED2009-05 }

sal_Bool ScMarkArray::IsAllMarked( SCROW nStartRow, SCROW nEndRow ) const
{
	SCSIZE nStartIndex;
	SCSIZE nEndIndex;

	if (Search( nStartRow, nStartIndex ))
		if (pData[nStartIndex].bMarked)
			if (Search( nEndRow, nEndIndex ))
				if (nEndIndex==nStartIndex)
					return sal_True;

	return sal_False;
}

sal_Bool ScMarkArray::HasOneMark( SCROW& rStartRow, SCROW& rEndRow ) const
{
	sal_Bool bRet = sal_False;
	if ( nCount == 1 )
	{
		if ( pData[0].bMarked )
		{
			rStartRow = 0;
			rEndRow = MAXROW;
			bRet = sal_True;
		}
	}
	else if ( nCount == 2 )
	{
		if ( pData[0].bMarked )
		{
			rStartRow = 0;
			rEndRow = pData[0].nRow;
		}
		else
		{
			rStartRow = pData[0].nRow + 1;
			rEndRow = MAXROW;
		}
		bRet = sal_True;
	}
	else if ( nCount == 3 )
	{
		if ( pData[1].bMarked )
		{
			rStartRow = pData[0].nRow + 1;
			rEndRow = pData[1].nRow;
			bRet = sal_True;
		}
	}
	return bRet;
}

void ScMarkArray::CopyMarksTo( ScMarkArray& rDestMarkArray ) const
{
	delete[] rDestMarkArray.pData;

	if (pData)
	{
		rDestMarkArray.pData = new ScMarkEntry[nCount];
		memmove( rDestMarkArray.pData, pData, nCount * sizeof(ScMarkEntry) );
	}
	else
		rDestMarkArray.pData = NULL;

    rDestMarkArray.nCount = rDestMarkArray.nLimit = nCount;
}

SCsROW ScMarkArray::GetNextMarked( SCsROW nRow, sal_Bool bUp ) const
{
    if (!pData)
        const_cast<ScMarkArray*>(this)->Reset(sal_False);   // create pData for further processing

	SCsROW nRet = nRow;
	if (VALIDROW(nRow))
	{
		SCSIZE nIndex;
		Search(nRow, nIndex);
		if (!pData[nIndex].bMarked)
		{
			if (bUp)
			{
				if (nIndex>0)
					nRet = pData[nIndex-1].nRow;
				else
					nRet = -1;
			}
			else
				nRet = pData[nIndex].nRow + 1;
		}
	}
	return nRet;
}

SCROW ScMarkArray::GetMarkEnd( SCROW nRow, sal_Bool bUp ) const
{
    if (!pData)
        const_cast<ScMarkArray*>(this)->Reset(sal_False);   // create pData for further processing

	SCROW nRet;
	SCSIZE nIndex;
	Search(nRow, nIndex);
	DBG_ASSERT( pData[nIndex].bMarked, "GetMarkEnd ohne bMarked" );
	if (bUp)
	{
		if (nIndex>0)
			nRet = pData[nIndex-1].nRow + 1;
		else
			nRet = 0;
	}
	else
		nRet = pData[nIndex].nRow;

	return nRet;
}

//
//	-------------- Iterator ----------------------------------------------
//

ScMarkArrayIter::ScMarkArrayIter( const ScMarkArray* pNewArray ) :
	pArray( pNewArray ),
	nPos( 0 )
{
}

ScMarkArrayIter::~ScMarkArrayIter()
{
}

sal_Bool ScMarkArrayIter::Next( SCROW& rTop, SCROW& rBottom )
{
	if ( nPos >= pArray->nCount )
		return sal_False;
	while (!pArray->pData[nPos].bMarked)
	{
		++nPos;
		if ( nPos >= pArray->nCount )
			return sal_False;
	}
	rBottom = pArray->pData[nPos].nRow;
	if (nPos==0)
		rTop = 0;
	else
		rTop = pArray->pData[nPos-1].nRow + 1;
	++nPos;
	return sal_True;
}





