#include "HandControl.h"

// windows header
#include <Windows.h>

/**
 * Keyboard simulator
 */
void SendKey( WORD key )
{
	INPUT mWinEvent;
	mWinEvent.type = INPUT_KEYBOARD;
	mWinEvent.ki.time = 0;
	mWinEvent.ki.dwFlags = 0;
	mWinEvent.ki.wScan = 0;
	mWinEvent.ki.wVk = key;
	SendInput( 1, &mWinEvent, sizeof(mWinEvent) );
}

void QHandIcon::paint( QPainter *pPainter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
	//TODO: should process as parameter
	switch( m_eStatus )
	{
	case HS_GENERAL:
		{
			pPainter->setPen( m_penGeneral );
			pPainter->drawArc( m_Rect, 0, 360 * 16 );
		}
		break;

	case HS_FIXING:
		{
			pPainter->setPen( m_penFixingProgress );
			pPainter->drawArc( m_Rect, 90*16, 360 * 16 * m_fProgress );

			pPainter->setPen( m_penFixing );
			pPainter->drawArc( m_Rect, 0, 360 * 16 );
		}
		break;

	case HS_FIXED:
		{
			pPainter->setPen( m_penFixed );
			pPainter->setBrush( m_brushFixed );
			pPainter->drawEllipse( m_Rect );
		}
		break;
	}
}

bool QHandControl::UpdateStatus( const QHandControl::EControlStatus& eStatus )
{
	if( m_eControlStatus != eStatus )
	{
		m_eControlStatus = eStatus;
		m_HandIcon.show();

		switch( m_eControlStatus )
		{
		case NICS_NO_HAND:
			HandReset();
			m_HandIcon.hide();
			m_qButtons.hide();
			break;

		case NICS_STANDBY:
			m_HandIcon.SetStatus( QHandIcon::HS_GENERAL );
			break;

		case NICS_FIXING:
			m_HandIcon.SetStatus( QHandIcon::HS_FIXING );
			m_FixPos = CurrentPos();
			break;

		case NICS_FIXED:
			m_FixPos = CurrentPos();
			m_HandIcon.SetStatus( QHandIcon::HS_FIXED );
			m_qButtons.show();
			break;

		case NICS_INPUT:
			m_FixPos = CurrentPos();
			
			break;
		}
		return true;
	}
	return false;
}

void QHandControl::UpdateHandPoint( const QPointF& rPt2D, const QVector3D& rPt3D )
{
	// add to points list
	SHandPos mPos( rPt2D, rPt3D );
	m_aTrackList.push_back( mPos );

	// move hand icon
	m_HandIcon.resetTransform();
	m_HandIcon.translate( rPt2D.x(), rPt2D.y() );

	// process
	if( m_eControlStatus == NICS_NO_HAND )
		UpdateStatus( NICS_STANDBY );
	
	if( m_eControlStatus == NICS_STANDBY )
	{
		if( rPt3D.z() < -m_fHandForwardDistance )
		{
			// start float hand button if fix
			if( Is2DPosFixFor( m_tdPreFixTime, m_fHandMoveThreshold ) )
			{
				UpdateStatus( NICS_FIXING );
			}
		}
	}

	// check if hand move out from button
	if( m_eControlStatus == NICS_FIXING )
	{
		if( QLineF( m_FixPos.mPos2D, rPt2D ).length() > m_fHandMoveThreshold )
		{
			UpdateStatus( NICS_STANDBY );
		}
		else
		{
			float fProgress = float(boost::chrono::duration_cast<boost::chrono::milliseconds>( mPos.tpTime - m_FixPos.tpTime ).count()) / m_tdFixTime.count();
			m_HandIcon.SetProgress( fProgress );
			if( fProgress > 1 )
			{
				m_qButtons.resetTransform();
				m_qButtons.translate( rPt2D.x(), rPt2D.y() );
				UpdateStatus( NICS_FIXED );
			}
		}
	}

	if( m_eControlStatus == NICS_FIXED )
	{
		//TODO: Should avoid multiple invoke via hand shacking
		for( auto itBut = m_vButtons.begin(); itBut != m_vButtons.end(); ++ itBut )
		{
			QGraphicsItem* pItem = itBut->first;
			if( pItem->shape().contains( pItem->mapFromScene( rPt2D ) ) )
			{
				(itBut->second)();
				m_pCurrentButton = pItem;
				UpdateStatus( NICS_INPUT );
				break;
			}
		}
	}

	if( m_eControlStatus == NICS_INPUT )
	{
		if( !( m_pCurrentButton->shape().contains( m_pCurrentButton->mapFromScene( rPt2D ) ) ) )
		{
			m_eControlStatus = NICS_FIXED;
		}
	}
}

void QHandControl::BuildButtons()
{
	//TODO: should process as parameter
	QPixmap imgArrow = QPixmap( "next.png" ).scaled( QSize( 60, 60 ), Qt::KeepAspectRatio );

	QGraphicsPixmapItem* pNext = new QGraphicsPixmapItem( imgArrow );
	pNext->translate( 50, -30 );
	m_qButtons.addToGroup( pNext );
	m_vButtons.push_back( std::pair<QGraphicsItem*,std::function<void()> >( pNext, [](){
		std::cout << "NEXT" << std::endl;
		SendKey( VK_NEXT );
	} ) );

	QGraphicsPixmapItem* pPerv = new QGraphicsPixmapItem( imgArrow );
	pPerv->rotate( 180 );
	pPerv->translate( 50, -30 );
	m_qButtons.addToGroup( pPerv );
	m_vButtons.push_back( std::pair<QGraphicsItem*,std::function<void()> >( pPerv, [](){
		std::cout << "PERV" << std::endl;
		SendKey( VK_PRIOR );
	} ) );
}