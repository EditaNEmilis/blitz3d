
#include "std.h"
#include "entity.h"

// The per-entity profiling header this used to include (stats.h) is not part
// of the open source release, and nothing here needs it.

Entity *Entity::_orphans,*Entity::_last_orphan;

enum{
	INVALID_LOCALTFORM=1,
	INVALID_WORLDTFORM=2,
	INVALID_WORLDROT=4,
	INVALID_WORLDSCL=8
};

//the three world caches are guarded by three bits, not one. Sharing a bit
//means whichever getter runs first clears it and the others read a stale
//value - EntityPitch(e,1) followed by EntityX(e,1) would return the
//position from before the last move.
enum{
	INVALID_WORLD_ALL=INVALID_WORLDTFORM|INVALID_WORLDROT|INVALID_WORLDSCL
};

void Entity::remove(){
	if( _parent ){
		if( _parent->_children==this ) _parent->_children=_succ;
		if( _parent->_last_child==this ) _parent->_last_child=_pred;
	}else{
		if( _orphans==this ) _orphans=_succ;
		if( _last_orphan==this ) _last_orphan=_pred;
	}
	if( _succ ) _succ->_pred=_pred;
	if( _pred ) _pred->_succ=_succ;
}

void Entity::insert(){
	_succ=0;
	if( _parent ){
		if( _pred=_parent->_last_child ) _pred->_succ=this;
		else _parent->_children=this;
		_parent->_last_child=this;
	}else{
		if( _pred=_last_orphan ) _pred->_succ=this;
		else _orphans=this;
		_last_orphan=this;
	}
}

Entity::Entity():
_succ(0),_pred(0),_parent(0),_children(0),_last_child(0),
_visible(true),_enabled(true),
local_scl(1,1,1),
//every cache starts invalid. They used to default to "valid" and hand back
//the default-constructed members, which for world_scl is (0,0,0) where the
//answer is local_scl - getWorldRotation and getWorldScale recomputed every
//call so nothing noticed. Memoised caches have to start invalid instead.
//The value each recompute produces is bit-for-bit what the members already
//held, so this is not an observable change on its own.
invalid( INVALID_LOCALTFORM|INVALID_WORLD_ALL ){
	insert();
}

Entity::Entity( const Entity &e ):
_succ(0),_pred(0),_parent(0),_children(0),_last_child(0),
_name(e._name),_visible(e._visible),_enabled(e._enabled),
local_pos(e.local_pos),
local_scl(e.local_scl),
local_rot(e.local_rot),
invalid( INVALID_LOCALTFORM|INVALID_WORLD_ALL ){
	insert();
}

Entity::~Entity(){
	while( children() ) delete children();
	remove();
}

void Entity::invalidateWorld(){
	//The early-out has to be an ALL-of test, not an ANY-of one. A getter
	//that has cleared one of the three world bits leaves the other two set,
	//so an ANY-of guard returns early here and never puts the cleared bit
	//back - and the next getWorldTform() then hands out a stale transform
	//instead of recomputing it. The set below is unconditional for the same
	//reason: it is what puts a partially-valid entity back to fully invalid.
	if( (invalid&INVALID_WORLD_ALL)==INVALID_WORLD_ALL ) return;
	invalid|=INVALID_WORLD_ALL;
	for( Entity *e=_children;e;e=e->_succ ){
		e->invalidateWorld();
	}
}

void Entity::invalidateLocal(){
	invalid|=INVALID_LOCALTFORM;
	invalidateWorld();
}

const Transform &Entity::getLocalTform()const{
	if( invalid&INVALID_LOCALTFORM ){
		local_tform.m=Matrix( local_rot );
		local_tform.m.i*=local_scl.x;
		local_tform.m.j*=local_scl.y;
		local_tform.m.k*=local_scl.z;
		local_tform.v=local_pos;
		invalid&=~INVALID_LOCALTFORM;
	}
	return local_tform;
}

const Transform &Entity::getWorldTform()const{
	if( invalid&INVALID_WORLDTFORM ){
		world_tform=_parent ? _parent->getWorldTform() * getLocalTform() : getLocalTform();
		invalid&=~INVALID_WORLDTFORM;
	}
	return world_tform;
}

void Entity::setParent( Entity *p ){
	if( _parent==p ) return;

	remove();

	_parent=p;

	insert();

	invalidateWorld();
}

void Entity::setName( const string &t ){
	_name=t;
}

void Entity::setVisible( bool visible ){
	_visible=visible;
}

void Entity::setEnabled( bool enabled ){
	_enabled=enabled;
}

void Entity::enumVisible( vector<Object*> &out ){
	if( !_visible ) return;
	if( Object *o=getObject() ) out.push_back(o);
	for( Entity *e=_children;e;e=e->_succ ){
		e->enumVisible( out );
	}
}

void Entity::enumEnabled( vector<Object*> &out ){
	if( !_enabled ) return;
	if( Object *o=getObject() ) out.push_back(o);
	for( Entity *e=_children;e;e=e->_succ ){
		e->enumEnabled( out );
	}
}

void Entity::setLocalPosition( const Vector &v ){
	local_pos=v;
	invalidateLocal();
}

void Entity::setLocalScale( const Vector &v ){
	local_scl=v;
	invalidateLocal();
}

void Entity::setLocalRotation( const Quat &q ){
	local_rot=q.normalized();
	invalidateLocal();
}

void Entity::setLocalTform( const Transform &t ){
	local_pos=t.v;
	local_scl=Vector( t.m.i.length(),t.m.j.length(),t.m.k.length() );
	local_rot=matrixQuat( t.m );
	invalidateLocal();
}

void Entity::setWorldPosition( const Vector &v ){
	setLocalPosition( _parent ? -_parent->getWorldTform() * v : v );
}

void Entity::setWorldScale( const Vector &v ){
	setLocalScale( _parent ? v/_parent->getWorldScale() : v );
}

void Entity::setWorldRotation( const Quat &q ){
	setLocalRotation( _parent ? -_parent->getWorldRotation() * q : q );
}

void Entity::setWorldTform( const Transform &t ){
	setLocalTform( _parent ? -_parent->getWorldTform() * t : t );
}

const Vector &Entity::getLocalPosition()const{
	return local_pos;
}

const Vector &Entity::getLocalScale()const{
	return local_scl;
}

const Quat &Entity::getLocalRotation()const{
	return local_rot;
}

const Vector &Entity::getWorldPosition()const{
	return getWorldTform().v;
}

const Vector &Entity::getWorldScale()const{
	//memoised. invalidateWorld is the only writer of INVALID_WORLDSCL and
	//every path that changes local_scl goes through
	//setLocalScale/invalidateLocal, so this is pure caching - it used to walk
	//and re-multiply the whole parent chain on every single read.
	if( invalid&INVALID_WORLDSCL ){
		world_scl=_parent ? _parent->getWorldScale() * local_scl : local_scl;
		invalid&=~INVALID_WORLDSCL;
	}
	return world_scl;
}

const Quat &Entity::getWorldRotation()const{
	if( invalid&INVALID_WORLDROT ){
		world_rot=_parent ? _parent->getWorldRotation() * local_rot : local_rot;
		invalid&=~INVALID_WORLDROT;
	}
	return world_rot;
}
