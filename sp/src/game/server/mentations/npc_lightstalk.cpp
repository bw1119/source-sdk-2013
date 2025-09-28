//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "sprite.h"
#include "baseanimating.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ndebugoverlay.h"

enum LightstalkFade_t // this is needed for fading the light effects and the glow when distant
{
	FADE_NONE,
	FADE_OUT,
	FADE_IN,
};

class CSprite; // for the glow

// the light fungi
class CNPC_LightFungi : public CAI_BaseNPC
{
public:
	DECLARE_CLASS(CNPC_LightFungi, CAI_BaseNPC)
	CNPC_LightFungi(void);
	void Spawn(void);
	void Precache(void);
	void Think(void);

	color32		m_LightColor;
	CNetworkVar(int, m_bLight); // turn on, turn off
	CNetworkVar(int, m_lightr); // red channel
	CNetworkVar(int, m_lightg); // green channel
	CNetworkVar(int, m_lightb); // blue channel
	CNetworkVar(float, m_LightRadius); // radius for dyn. light
	int			m_lighta;		// brightness
private:
	float				m_flStartFadeTime;
	LightstalkFade_t	m_nFadeDir;
	CSprite* m_pGlow;
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};

#define	FUNGI_MODEL				"models/fungus_light.mdl"
#define FUNGI_GLOW_SPRITE		"sprites/glow03.vmt"

LINK_ENTITY_TO_CLASS(npc_lightfungi, CNPC_LightFungi);

BEGIN_DATADESC(CNPC_LightFungi)

DEFINE_FIELD(m_pGlow, FIELD_CLASSPTR),
DEFINE_FIELD(m_flStartFadeTime, FIELD_TIME),
DEFINE_FIELD(m_nFadeDir, FIELD_INTEGER),
//DEFINE_KEYFIELD(m_lightr, FIELD_INTEGER, "light_r"),
//DEFINE_KEYFIELD(m_lightg, FIELD_INTEGER, "light_g"),
//DEFINE_KEYFIELD(m_lightb, FIELD_INTEGER, "light_b"),
//DEFINE_KEYFIELD(m_lighta, FIELD_INTEGER, "light_a"),
DEFINE_KEYFIELD(m_LightColor, FIELD_COLOR32, "lightcolor"), // this keyvalue corresponds with the fgd entry
DEFINE_KEYFIELD(m_LightRadius, FIELD_FLOAT, "radius"), // this keyvalue corresponds with the fgd entry

// Function Pointers
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CNPC_LightFungi, DT_NPC_LightFungi)
SendPropInt(SENDINFO(m_bLight), 1, SPROP_UNSIGNED), // we're sending it to the client because dyn. light is handled there
SendPropInt(SENDINFO(m_lightr), 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_lightg), 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_lightb), 1, SPROP_UNSIGNED),
SendPropFloat(SENDINFO(m_LightRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()

CNPC_LightFungi::CNPC_LightFungi(void)
{
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightFungi::Precache(void)
{
	engine->PrecacheModel(FUNGI_MODEL);
	engine->PrecacheModel(FUNGI_GLOW_SPRITE);

	BaseClass::Precache();
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightFungi::Spawn(void)
{
	Precache();
	SetModel(FUNGI_MODEL);

	m_lightr = m_LightColor.r; // we exctract these values from the keyvalue, which is set in Hammer
	m_lightg = m_LightColor.g;
	m_lightb = m_LightColor.b;
	m_lighta = m_LightColor.a;

	m_bLight = true; // turned on by default, and is generally always on for the fungi

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();
	SetMoveType(MOVETYPE_NONE);
	SetSolid(SOLID_VPHYSICS); // the model lacks the physmodel, though
//	AddSolidFlags(FSOLID_TRIGGER);
//	AddSolidFlags(FSOLID_NOT_SOLID);

//	UTIL_SetSize(this, Vector(-80, -80, 0), Vector(80, 80, 32));
	SetModelScale(RandomFloat(0.75, 1.5), 0.0f); // we randomise the size so it's not always clones of the one model
	UpdateModelScale();
	SetActivity(ACT_IDLE);
	SetNextThink(gpGlobals->curtime + 0.1f);

	m_flPlaybackRate = random->RandomFloat(0.5, 1.5); // we also randomise the animation rate so they don't "breathe" in sync with each other--
	SetCycle(random->RandomFloat(0.0f, 0.9f));	// more visual variety when put in clusters

	m_pGlow = CSprite::SpriteCreate(FUNGI_GLOW_SPRITE, GetLocalOrigin() + Vector(0, 0, (WorldAlignMins().z + WorldAlignMaxs().z)*0.5), false);
	m_pGlow->SetTransparency(kRenderGlow, m_lightr, m_lightg, m_lightb, m_lighta, kRenderFxGlowShell);
	m_pGlow->SetScale(2.0);
	m_pGlow->SetAttachment(this, 1); // attachment defined in the qc

	BaseClass::Spawn();
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightFungi::Think(void)
{
	StudioFrameAdvance();
	SetNextThink(gpGlobals->curtime + 0.1f);

	switch (GetActivity())
	{
	case ACT_IDLE:
	default:
		break;
	}

	if (m_nFadeDir && m_pGlow)
	{
		float flFadePercent = (gpGlobals->curtime - m_flStartFadeTime) / 2;

		if (flFadePercent > 1)
		{
			m_nFadeDir = FADE_NONE;
		}
		else
		{
			if (m_nFadeDir == FADE_IN)
			{
				m_pGlow->SetBrightness(120*flFadePercent);
				m_bLight = true;
			}
			else
			{
				//m_pGlow->SetBrightness(LIGHTSTALK_MAX_BRIGHT*(1-flFadePercent));
				// Fade out immedately
				m_pGlow->SetBrightness(0);
				m_bLight = false;
			}
		}
	}
}

// the lightstalk
class CNPC_LightStalk : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_LightStalk, CAI_BaseNPC );

	CNPC_LightStalk( void );

	void		Spawn( void );
	void		Precache( void );
	void		Touch( CBaseEntity *pOther );
	void		Think( void );
	void		LightRise( void );
	void		LightLower( void );

	COutputEvent m_OnRise;
	COutputEvent m_OnLower;

	color32		m_LightColor;
	
	CNetworkVar( int, m_bLight ); // again, we network these into the client
	CNetworkVar( int, m_lightr );
	CNetworkVar( int, m_lightg );
	CNetworkVar( int, m_lightb );
	CNetworkVar(float, m_LightRadius); // radius for dyn. light

	int			m_lighta;

private:
	float				m_flHideEndTime;
	float				m_flStartFadeTime;
	LightstalkFade_t	m_nFadeDir;
	CSprite*			m_pGlow;

public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};

#define	LIGHTSTALK_MODEL			"models/light.mdl"
#define LIGHTSTALK_GLOW_SPRITE		"sprites/glow03.vmt"
#define LIGHTSTALK_HIDE_TIME		5
#define	LIGHTSTALK_FADE_TIME		2
#define LIGHTSTALK_MAX_BRIGHT		120

LINK_ENTITY_TO_CLASS(npc_lightstalk, CNPC_LightStalk);

BEGIN_DATADESC( CNPC_LightStalk )

	DEFINE_FIELD( m_pGlow,				FIELD_CLASSPTR ),
	DEFINE_FIELD( m_flStartFadeTime,	FIELD_TIME ),
	DEFINE_FIELD( m_nFadeDir,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flHideEndTime,		FIELD_TIME ),
//	DEFINE_KEYFIELD( m_lightr,			FIELD_INTEGER, "light_r" ),
//	DEFINE_KEYFIELD( m_lightg,			FIELD_INTEGER, "light_g" ),
//	DEFINE_KEYFIELD( m_lightb,			FIELD_INTEGER, "light_b" ),
//	DEFINE_KEYFIELD( m_lighta,			FIELD_INTEGER, "light_a" ),
	DEFINE_KEYFIELD( m_LightColor,		FIELD_COLOR32, "lightcolor" ),
	DEFINE_KEYFIELD(m_LightRadius,		FIELD_FLOAT, "radius"), // this keyvalue corresponds with the fgd entry

	// outputs
	DEFINE_OUTPUT( m_OnRise,  "OnRise" ),
	DEFINE_OUTPUT( m_OnLower, "OnLower" ),

	// Function Pointers
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CNPC_LightStalk, DT_NPC_LightStalk )
	SendPropInt( SENDINFO( m_bLight ), 1, SPROP_UNSIGNED ), // these go to the client to be handled there
	SendPropInt( SENDINFO( m_lightr ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_lightg ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_lightb ), 1, SPROP_UNSIGNED ),
	SendPropFloat(SENDINFO(m_LightRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()

CNPC_LightStalk::CNPC_LightStalk( void )
{
//	m_LightColor.Init( 255,255,255,100);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Precache( void )
{
	engine->PrecacheModel( LIGHTSTALK_MODEL );
	engine->PrecacheModel( LIGHTSTALK_GLOW_SPRITE );

	BaseClass::Precache();
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Spawn( void )
{
	Precache();
	SetModel( LIGHTSTALK_MODEL );

	m_lightr = m_LightColor.r;
	m_lightg = m_LightColor.g;
	m_lightb = m_LightColor.b;
	m_lighta = m_LightColor.a;

	m_bLight = true;
	
	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER ); // this NPC serves as a trigger - makes reactions possible
	AddSolidFlags( FSOLID_NOT_SOLID );

	UTIL_SetSize( this, Vector(-80,-80,0), Vector(80,80,32)); // the size of the trigger. When the player touches this virtual box, stuff happens.
	SetActivity( ACT_IDLE );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flPlaybackRate = random->RandomFloat(0.5, 1.5); // randomise for more random idle animation playback
	SetCycle(random->RandomFloat(0.0f, 0.9f));
	
	m_pGlow = CSprite::SpriteCreate( LIGHTSTALK_GLOW_SPRITE, GetLocalOrigin() + Vector(0,0,(WorldAlignMins().z+WorldAlignMaxs().z)*0.5), false );
	m_pGlow->SetTransparency( kRenderGlow, m_lightr, m_lightg, m_lightb, m_lighta, kRenderFxGlowShell );
	m_pGlow->SetScale( 1.0 );
	m_pGlow->SetAttachment( this, 1 );
	LightRise();

	BaseClass::Spawn();
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Think( void )
{
	StudioFrameAdvance();
	SetNextThink( gpGlobals->curtime + 0.1f );

	switch( GetActivity() )
	{
	case ACT_CROUCH:
		if ( IsActivityFinished() )
		{
			SetActivity( ACT_CROUCHIDLE );
		}
		break;

	case ACT_CROUCHIDLE:
		if ( gpGlobals->curtime > m_flHideEndTime )
		{
			SetActivity( ACT_STAND );
			LightRise();
		}
		break;

	case ACT_STAND:
		if ( IsActivityFinished() )
		{
			SetActivity( ACT_IDLE );
		}
		break;

	case ACT_IDLE:
	default:
		break;
	}

	if (m_nFadeDir && m_pGlow)
	{
		float flFadePercent = (gpGlobals->curtime - m_flStartFadeTime)/LIGHTSTALK_FADE_TIME;

		if (flFadePercent > 1)
		{
			m_nFadeDir = FADE_NONE;
		}
		else
		{
			if  (m_nFadeDir == FADE_IN)
			{
				m_pGlow->SetBrightness(LIGHTSTALK_MAX_BRIGHT*flFadePercent);
				m_bLight = true;
			}
			else
			{
				//m_pGlow->SetBrightness(LIGHTSTALK_MAX_BRIGHT*(1-flFadePercent));
				// Fade out immedately
				m_pGlow->SetBrightness(0);
				m_bLight = false;
			}
		}
	}
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Touch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() ) // only react to the player
	{
		m_flHideEndTime = gpGlobals->curtime + LIGHTSTALK_HIDE_TIME;
		if ( GetActivity() == ACT_IDLE || GetActivity() == ACT_STAND )
		{
			SetActivity( ACT_CROUCH );
			LightLower();
		}
	}
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::LightRise( void )
{
	m_OnRise.FireOutput( this, this );
	m_nFadeDir			= FADE_IN;
	m_flStartFadeTime	= gpGlobals->curtime;
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::LightLower( void )
{
	m_OnLower.FireOutput( this, this );
	m_nFadeDir			= FADE_OUT;
	m_flStartFadeTime	= gpGlobals->curtime;
}