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


#include "precompiled_reportdesign.hxx"
#include "DateTime.hxx"
#ifndef RPTUI_DATETIME_HRC
#include "DateTime.hrc"
#endif
#include <com/sun/star/beans/XPropertySet.hpp>
#include <tools/debug.hxx>
#ifndef _RPTUI_DLGRESID_HRC
#include "RptResId.hrc"
#endif
#ifndef _RPTUI_SLOTID_HRC_
#include "rptui_slotid.hrc"
#endif
#ifndef _RPTUI_MODULE_HELPER_DBU_HXX_
#include "ModuleHelper.hxx"
#endif
#ifndef RTPUI_REPORTDESIGN_HELPID_HRC
#include "helpids.hrc"
#endif
#include <vcl/msgbox.hxx>
#ifndef _GLOBLMN_HRC
#include <svx/globlmn.hrc>
#endif
#ifndef _SBASLTID_HRC
#include <svx/svxids.hrc>
#endif
#include <connectivity/dbconversion.hxx>
#include <unotools/syslocale.hxx>
#ifndef RPTUI_TOOLS_HXX
#include "UITools.hxx"
#endif
#include "RptDef.hxx"
#ifndef REPORTDESIGN_SHARED_UISTRINGS_HRC
#include "uistrings.hrc"
#endif
#include "ReportController.hxx"
#include <com/sun/star/report/XFormattedField.hpp>
#include <com/sun/star/util/Time.hpp>
#include <com/sun/star/util/NumberFormat.hpp>
#include <com/sun/star/util/XNumberFormatPreviewer.hpp>
#include <com/sun/star/util/XNumberFormatTypes.hpp>
#include <com/sun/star/i18n/NumberFormatIndex.hpp>
#include <comphelper/numbers.hxx>
#include <algorithm>

namespace rptui
{
using namespace ::com::sun::star;
using namespace ::comphelper;

DBG_NAME( rpt_ODateTimeDialog )
//========================================================================
// class ODateTimeDialog
//========================================================================
ODateTimeDialog::ODateTimeDialog( Window* _pParent
										   ,const uno::Reference< report::XSection >& _xHoldAlive
										   ,OReportController* _pController)
	: ModalDialog( _pParent, ModuleRes(RID_DATETIME_DLG) )
	,m_aDate(this,	 		 ModuleRes(CB_DATE		) )
    ,m_aFTDateFormat(this,	 ModuleRes(FT_DATE_FORMAT	) )
	,m_aDateListBox(this,	 ModuleRes(LB_DATE_TYPE	) )
	,m_aFL0(this,            ModuleRes(FL_SEPARATOR0		) )
	,m_aTime(this,	         ModuleRes(CB_TIME		) )
    ,m_aFTTimeFormat(this,	 ModuleRes(FT_TIME_FORMAT ) )
	,m_aTimeListBox(this,	 ModuleRes(LB_TIME_TYPE	) )
	,m_aFL1(this,         ModuleRes(FL_SEPARATOR1) )
	,m_aPB_OK(this,		ModuleRes(PB_OK))
	,m_aPB_CANCEL(this,	ModuleRes(PB_CANCEL))
	,m_aPB_Help(this,	ModuleRes(PB_HELP))
	,m_aDateControlling()
	,m_aTimeControlling()
	,m_pController(_pController)
	,m_xHoldAlive(_xHoldAlive)
{
	DBG_CTOR( rpt_ODateTimeDialog,NULL);

	try
	{
		SvtSysLocale aSysLocale;
		m_nLocale = aSysLocale.GetLocaleData().getLocale();
	    // Fill listbox with all well known date types
        InsertEntry(util::NumberFormat::DATE);
        InsertEntry(util::NumberFormat::TIME);
    }
	catch(uno::Exception&)
	{
	}

	m_aDateListBox.SetDropDownLineCount(20);
	m_aDateListBox.SelectEntryPos(0);

	m_aTimeListBox.SetDropDownLineCount(20);
    m_aTimeListBox.SelectEntryPos(0);

	// use nice enhancement, to toggle enable/disable if a checkbox is checked or not
	m_aDateControlling.enableOnCheckMark( m_aDate, m_aFTDateFormat, m_aDateListBox);
	m_aTimeControlling.enableOnCheckMark( m_aTime, m_aFTTimeFormat, m_aTimeListBox);

	CheckBox* pCheckBoxes[] = { &m_aDate,&m_aTime};
	for ( size_t i = 0 ; i < sizeof(pCheckBoxes)/sizeof(pCheckBoxes[0]); ++i)
		pCheckBoxes[i]->SetClickHdl(LINK(this,ODateTimeDialog,CBClickHdl));

	FreeResource();
}
// -----------------------------------------------------------------------------
    void ODateTimeDialog::InsertEntry(sal_Int16 _nNumberFormatId)
    {
        const bool bTime = util::NumberFormat::TIME == _nNumberFormatId;
        ListBox* pListBox = &m_aDateListBox;
        if ( bTime )
            pListBox = &m_aTimeListBox;
        
        const uno::Reference< util::XNumberFormatter> xNumberFormatter = m_pController->getReportNumberFormatter();
	    const uno::Reference< util::XNumberFormats> xFormats = xNumberFormatter->getNumberFormatsSupplier()->getNumberFormats();
        const uno::Sequence<sal_Int32> aFormatKeys = xFormats->queryKeys(_nNumberFormatId,m_nLocale,sal_True);
        const sal_Int32* pIter = aFormatKeys.getConstArray();
        const sal_Int32* pEnd  = pIter + aFormatKeys.getLength();
        for(;pIter != pEnd;++pIter)
        {
            const sal_Int16 nPos = pListBox->InsertEntry(getFormatStringByKey(*pIter,xFormats,bTime));
            pListBox->SetEntryData(nPos, reinterpret_cast<void*>(*pIter));
        }
    }
//------------------------------------------------------------------------
ODateTimeDialog::~ODateTimeDialog()
{
	DBG_DTOR( rpt_ODateTimeDialog,NULL);	
}
// -----------------------------------------------------------------------------
short ODateTimeDialog::Execute()
{
	DBG_CHKTHIS( rpt_ODateTimeDialog,NULL);
	short nRet = ModalDialog::Execute();
	if ( nRet == RET_OK && (m_aDate.IsChecked() || m_aTime.IsChecked()) )
	{
		try
		{
            sal_Int32 nLength = 0;
            uno::Sequence<beans::PropertyValue> aValues( 6 );
            aValues[nLength].Name = PROPERTY_SECTION;
            aValues[nLength++].Value <<= m_xHoldAlive;

            aValues[nLength].Name = PROPERTY_TIME_STATE;
            aValues[nLength++].Value <<= m_aTime.IsChecked();

            aValues[nLength].Name = PROPERTY_DATE_STATE;
            aValues[nLength++].Value <<= m_aDate.IsChecked();

            aValues[nLength].Name = PROPERTY_FORMATKEYDATE;
            aValues[nLength++].Value <<= getFormatKey(sal_True);

            aValues[nLength].Name = PROPERTY_FORMATKEYTIME;
            aValues[nLength++].Value <<= getFormatKey(sal_False);

            sal_Int32 nWidth = 0;
            if ( m_aDate.IsChecked() )
            {
                String sDateFormat = m_aDateListBox.GetEntry( m_aDateListBox.GetSelectEntryPos() );
                nWidth = LogicToLogic(PixelToLogic(Size(GetCtrlTextWidth(sDateFormat),0)).Width(),GetMapMode().GetMapUnit(),MAP_100TH_MM);
            }
            if ( m_aTime.IsChecked() )
            {
                String sDateFormat = m_aTimeListBox.GetEntry( m_aTimeListBox.GetSelectEntryPos() );
                nWidth = ::std::max<sal_Int32>(LogicToLogic(PixelToLogic(Size(GetCtrlTextWidth(sDateFormat),0)).Width(),GetMapMode().GetMapUnit(),MAP_100TH_MM),nWidth);
            }

            if ( nWidth > 4000 )
            {
                aValues[nLength].Name = PROPERTY_WIDTH;
                aValues[nLength++].Value <<= nWidth;
            }

            m_pController->executeChecked(SID_DATETIME,aValues);
		}
		catch(uno::Exception&)
		{
			nRet = RET_NO;
		}
	}
	return nRet;
}
// -----------------------------------------------------------------------------
::rtl::OUString ODateTimeDialog::getFormatStringByKey(::sal_Int32 _nNumberFormatKey,const uno::Reference< util::XNumberFormats>& _xFormats,bool _bTime)
{
	uno::Reference< beans::XPropertySet> xFormSet = _xFormats->getByKey(_nNumberFormatKey);
	OSL_ENSURE(xFormSet.is(),"XPropertySet is null!");
	::rtl::OUString sFormat;
	xFormSet->getPropertyValue(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("FormatString"))) >>= sFormat;

	double nValue = 0;
	if ( _bTime )
	{
		Time aCurrentTime;
		nValue = ::dbtools::DBTypeConversion::toDouble(::dbtools::DBTypeConversion::toTime(aCurrentTime.GetTime()));
	}
	else
	{
		Date aCurrentDate;
		static ::com::sun::star::util::Date STANDARD_DB_DATE(30,12,1899);
		nValue = ::dbtools::DBTypeConversion::toDouble(::dbtools::DBTypeConversion::toDate(static_cast<sal_Int32>(aCurrentDate.GetDate())),STANDARD_DB_DATE);
	}

	uno::Reference< util::XNumberFormatPreviewer> xPreViewer(m_pController->getReportNumberFormatter(),uno::UNO_QUERY);
	OSL_ENSURE(xPreViewer.is(),"XNumberFormatPreviewer is null!");
	return xPreViewer->convertNumberToPreviewString(sFormat,nValue,m_nLocale,sal_True);
}
// -----------------------------------------------------------------------------
IMPL_LINK( ODateTimeDialog, CBClickHdl, CheckBox*, _pBox )
{
   (void)_pBox;
	DBG_CHKTHIS( rpt_ODateTimeDialog,NULL);

 	if ( _pBox == &m_aDate || _pBox == &m_aTime)
 	{
 		sal_Bool bDate = m_aDate.IsChecked();
		sal_Bool bTime = m_aTime.IsChecked();
		if (!bDate && !bTime)
		{
			m_aPB_OK.Disable();
		}
		else
		{
			m_aPB_OK.Enable();
		}
	}
	return 1L;
}
// -----------------------------------------------------------------------------
sal_Int32 ODateTimeDialog::getFormatKey(sal_Bool _bDate) const
{
    DBG_CHKTHIS( rpt_ODateTimeDialog,NULL);
	sal_Int32 nFormatKey;
	if ( _bDate )
    {
         // 	nFormat = m_aDateF1.IsChecked() ? i18n::NumberFormatIndex::DATE_SYSTEM_LONG : (m_aDateF2.IsChecked() ? i18n::NumberFormatIndex::DATE_SYS_DMMMYYYY : i18n::NumberFormatIndex::DATE_SYSTEM_SHORT);
         nFormatKey = static_cast<sal_Int32>(reinterpret_cast<sal_IntPtr>(m_aDateListBox.GetEntryData( m_aDateListBox.GetSelectEntryPos() )));
    }
	else
	{
        // 	nFormat = m_aTimeF1.IsChecked() ? i18n::NumberFormatIndex::TIME_HHMMSS : (m_aTimeF2.IsChecked() ? i18n::NumberFormatIndex::TIME_HHMMSSAMPM : i18n::NumberFormatIndex::TIME_HHMM);
         nFormatKey = static_cast<sal_Int32>(reinterpret_cast<sal_IntPtr>(m_aTimeListBox.GetEntryData( m_aTimeListBox.GetSelectEntryPos() )));
    }
	return nFormatKey;
}
// =============================================================================
} // rptui
// =============================================================================
