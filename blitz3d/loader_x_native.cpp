
#include "std.h"
#include "loader_x.h"
#include "meshmodel.h"
#include "animation.h"
#include "pivot.h"
#include "xfile.h"

//The loader for the DirectX 3D file format.
//
//This is the reader that blitz3d/loader_x.cpp was written against, with
//the DirectX file library taken out. That file needs <dxfile.h>,
//<rmxftmpl.h> and <rmxfguid.h> from the DirectX SDK, which Microsoft no
//longer distributes, and d3dxof.dll at run time, which is not installed on
//any current Windows. So the tree it walked, which the SDK's templates
//described, is walked here instead: the file is parsed into template
//instances and the same templates are read back in the same order.
//
//The templates that are read, and the order of their members, are:
//
//  Mesh               nVertices, vertices, nFaces, faces
//  MeshNormals        nNormals, normals, nFaceNormals, faceNormals
//  MeshTextureCoords  nTextureCoords, textureCoords
//  MeshVertexColors   nVertexColors, indices, colors
//  MeshMaterialList   nMaterials, nFaceIndexes, faceIndexes
//  Material           faceColor, power, specularColor, emissiveColor
//  TextureFilename    filename
//  FrameTransformMatrix  matrix
//  AnimationKey       keyType, nKeys, keys
//
//A frame's name and the name an animation refers to a frame by are the
//instance names, and a material can be written either inside the mesh's
//material list or as a reference to a material named earlier in the file.

static map<string,MeshModel*> frames_map;
static int anim_len;

static bool conv,flip_tris;
static Transform conv_tform;
static bool collapse,animonly;

static XFile *xfile;

//The keys of an animation. A key is a rotation, a scale or a position, and
//the rotation is a quaternion with the scalar part first, which is the
//order the format writes and the order the engine's own quaternion uses.
static void parseAnimKey( XNode *node,MeshModel *e ){
	XCursor c( node );

	int type=c.readInt();
	int cnt=c.readInt();

	Animation anim=e->getAnimation();
	for( int k=0;k<cnt;++k ){
		int time=c.readInt();
		int n=c.readInt();
		if( time>anim_len ) anim_len=time;
		int used=0;
		switch( type ){
		case 0:
			if( n==4 ){
				Quat rot;
				rot.w=c.readFloat();
				rot.v=Vector( c.readFloat(),c.readFloat(),c.readFloat() );
				used=4;
				if( conv ){
					if( fabs(rot.w)<1-EPSILON ){
						rot.normalize();
						//quat-to-axis/angle
						float half=acosf( rot.w );
						if( flip_tris ) half=-half;
						rot=Quat( cosf( half ),(conv_tform.m*rot.v).normalized()*sinf( half ) );
					}else rot=Quat();
				}
				anim.setRotationKey( time,rot );
			}
			break;
		case 1:
			if( n==3 ){
				Vector scl( c.readFloat(),c.readFloat(),c.readFloat() );
				used=3;
				if( conv ) scl=conv_tform.m * scl;
				scl.x=fabs(scl.x);scl.y=fabs(scl.y);scl.z=fabs(scl.z);
				anim.setScaleKey( time,scl );
			}
			break;
		case 2:
			if( n==3 ){
				Vector pos( c.readFloat(),c.readFloat(),c.readFloat() );
				used=3;
				if( conv ) pos=conv_tform*pos;
				anim.setPositionKey( time,pos );
			}
			break;
		}
		//any values the template did not use are skipped
		for( int j=used;j<n;++j ) c.readFloat();
	}
	e->setAnimation( anim );
}

//The animation of one frame. The frame the keys belong to is named by a
//reference, and the reference can come after the keys, so the keys are
//held until the frame is known.
static void parseAnim( XNode *node ){
	MeshModel *frame=0;
	vector<XNode*> keys;

	for( int k=0;k<(int)node->children.size();++k ){
		XNode *child=node->children[k];
		if( child->reference ){
			map<string,MeshModel*>::iterator it=frames_map.find( child->name );
			if( it!=frames_map.end() ) frame=it->second;
		}else if( child->name=="AnimationKey" ){
			keys.push_back( child );
		}
	}

	for( int k=0;k<(int)keys.size();++k ){
		if( frame ) parseAnimKey( keys[k],frame );
	}
}

static void parseAnimSet( XNode *node ){
	for( int k=0;k<(int)node->children.size();++k ){
		XNode *child=node->children[k];
		if( !child->reference && child->name=="Animation" ) parseAnim( child );
	}
}

static Brush parseMaterial( XNode *node ){
	Brush brush;

	XCursor c( node );

	float r=c.readFloat();
	float g=c.readFloat();
	float b=c.readFloat();
	float a=c.readFloat();

	brush.setColor( Vector( r,g,b ) );
	if( a ) brush.setAlpha( a );

	for( int k=0;k<(int)node->children.size();++k ){
		XNode *child=node->children[k];
		if( child->reference || child->name!="TextureFilename" ) continue;
		XCursor tc( child );
		string file=tc.readString();
		if( file.size() ){
			brush.setTexture( 0,Texture( file,0 ),0 );
			brush.setColor( Vector( 1,1,1 ) );
		}
	}

	return brush;
}

static void parseMaterialList( XNode *node,vector<Brush> &mats ){
	for( int k=0;k<(int)node->children.size();++k ){
		XNode *child=node->children[k];
		XNode *mat=child;
		if( child->reference ){
			//a material named earlier in the file
			mat=xfile->find( child->name );
			if( !mat ) continue;
		}
		if( mat->name=="Material" ) mats.push_back( parseMaterial( mat ) );
	}
}

static void parseMesh( XNode *node,MeshModel *mesh ){
	XCursor c( node );

	vector<Brush> mats;

	//the faces, flattened: the number of indices in the face, then the
	//indices themselves
	vector<int> face_data;
	vector<int> face_at;

	MeshLoader::beginMesh();

	int num_verts=c.readInt();
	for( int k=0;k<num_verts;++k ){
		Surface::Vertex v;
		v.coords=Vector( c.readFloat(),c.readFloat(),c.readFloat() );
		if( conv ) v.coords=conv_tform * v.coords;
		v.color=0xffffffff;
		MeshLoader::addVertex( v );
	}

	int num_faces=c.readInt();
	for( int k=0;k<num_faces;++k ){
		face_at.push_back( face_data.size() );
		int cnt=c.readInt();
		face_data.push_back( cnt );
		for( int j=0;j<cnt;++j ) face_data.push_back( c.readInt() );
	}

	//the material of each face
	vector<int> face_mat( num_faces,0 );

	bool normals=false;

	for( int k=0;k<(int)node->children.size();++k ){
		XNode *child=node->children[k];
		if( child->reference ) continue;

		if( child->name=="MeshMaterialList" ){
			XCursor mc( child );
			mc.readInt();//the number of materials
			int num_face_mats=mc.readInt();
			for( int j=0;j<num_face_mats;++j ){
				int index=mc.readInt();
				if( j<num_faces ) face_mat[j]=index;
			}
			parseMaterialList( child,mats );
		}else if( child->name=="MeshTextureCoords" ){
			XCursor tc( child );
			int num_coords=tc.readInt();
			if( num_coords==num_verts ){
				for( int j=0;j<num_coords;++j ){
					Surface::Vertex &v=MeshLoader::refVertex(j);
					float tu=tc.readFloat(),tv=tc.readFloat();
					v.tex_coords[0][0]=v.tex_coords[1][0]=tu;
					v.tex_coords[0][1]=v.tex_coords[1][1]=tv;
				}
			}
		}else if( child->name=="MeshVertexColors" ){
			XCursor vc( child );
			int num_colors=vc.readInt();
			if( num_colors==num_verts ){
				for( int j=0;j<num_colors;++j ){
					int index=vc.readInt();
					float r=vc.readFloat(),g=vc.readFloat(),b=vc.readFloat(),a=vc.readFloat();
					if( index<0 || index>=num_verts ) continue;
					Surface::Vertex &v=MeshLoader::refVertex(index);
					v.color=0xff000000|(int(r*255)<<16)|(int(g*255)<<8)|int(b*255);
				}
			}
		}else if( child->name=="MeshNormals" ){
			XCursor nc( child );
			int num_normals=nc.readInt();
			if( num_normals==num_verts ){
				Matrix co=conv_tform.m.cofactor();
				for( int j=0;j<num_normals;++j ){
					Vector n( nc.readFloat(),nc.readFloat(),nc.readFloat() );
					Surface::Vertex &v=MeshLoader::refVertex(j);
					v.normal=(co * n).normalized();
				}
				normals=true;
			}
		}
	}

	if( !mats.size() ) mats.push_back( Brush() );

	for( int k=0;k<num_faces;++k ){
		int at=face_at[k];
		int cnt=face_data[at];
		if( cnt<3 ) continue;
		int mat=face_mat[k];
		if( mat<0 || mat>=(int)mats.size() ) mat=0;
		int tri[3];
		tri[0]=face_data[at+1];
		for( int j=2;j<cnt;++j ){
			tri[1]=face_data[at+j+flip_tris];
			tri[2]=face_data[at+j+1-flip_tris];
			MeshLoader::addTriangle( tri,mats[mat] );
		}
	}

	MeshLoader::endMesh( mesh );

	if( !normals ) mesh->updateNormals();
}

static MeshModel *parseFrame( XNode *node ){
	MeshModel *e=d_new MeshModel();

	if( node->inst.size() ){
		e->setName( node->inst );
		frames_map[node->inst]=e;
	}

	for( int k=0;k<(int)node->children.size();++k ){
		XNode *child=node->children[k];
		if( child->reference ) continue;

		if( child->name=="FrameTransformMatrix" ){
			XCursor c( child );
			//a row major 4x4 matrix, the translation in the last row
			float m[16];
			for( int j=0;j<16;++j ) m[j]=c.readFloat();
			Transform tform=Transform( Matrix(
				Vector( m[0],m[1],m[2] ),
				Vector( m[4],m[5],m[6] ),
				Vector( m[8],m[9],m[10] ) ),
				Vector( m[12],m[13],m[14] ) );
			if( conv ) tform=conv_tform * tform * -conv_tform;
			e->setLocalTform( tform );
		}else if( child->name=="Mesh" ){
			if( !animonly ) parseMesh( child,e );
		}else if( child->name=="Frame" ){
			MeshModel *t=parseFrame( child );
			t->setParent( e );
		}
	}
	return e;
}

static MeshModel *parseFile( const string &file ){
	anim_len=0;

	xfile=XFile::load( file );
	if( !xfile ) return 0;

	MeshModel *e=d_new MeshModel();

	const vector<XNode*> &roots=xfile->getRoots();
	for( int k=0;k<(int)roots.size();++k ){
		XNode *node=roots[k];
		if( node->reference ) continue;

		if( node->name=="Mesh" ){
			if( !animonly ) parseMesh( node,e );
		}else if( node->name=="Frame" ){
			MeshModel *t=parseFrame( node );
			t->setParent( e );
		}else if( node->name=="AnimationSet" ){
			if( !collapse ) parseAnimSet( node );
		}
	}

	if( !collapse ){
		e->setAnimator( d_new Animator( e,anim_len ) );
	}

	return e;
}

MeshModel *Loader_X::load( const string &filename,const Transform &t,int hint ){
	conv_tform=t;
	conv=flip_tris=false;
	if( conv_tform!=Transform() ){
		conv=true;
		if( conv_tform.m.i.cross(conv_tform.m.j).dot(conv_tform.m.k)<0 ) flip_tris=true;
	}
	collapse=!!(hint&MeshLoader::HINT_COLLAPSE);
	animonly=!!(hint&MeshLoader::HINT_ANIMONLY);

	frames_map.clear();
	MeshModel *e=parseFile( filename );
	frames_map.clear();
	delete xfile;
	xfile=0;

	return e;
}
