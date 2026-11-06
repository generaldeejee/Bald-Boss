package com.baldelite.game;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Random;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

public final class GameRenderer implements GLSurfaceView.Renderer {
    private final GameState game;
    private final Context context;
    private int surfaceWidth=960,surfaceHeight=540;
    private int faceProgram,hairProgram,backgroundProgram,faceTexture,backgroundTexture;
    private FloatBuffer faceVertices,hairVertices,backgroundVertices;
    private int faceCount=0,hairCount=0,lastCandidate=-1;
    private final float[] projection=new float[16],view=new float[16],model=new float[16],mv=new float[16],mvp=new float[16];
    private long lastTime=0;
    private static final String FACE_VERT=
        "attribute vec3 aPosition; attribute vec2 aUV; attribute vec3 aNormal;\n"+
        "uniform mat4 uMVP; uniform mat4 uModel;\n"+
        "varying vec2 vUV; varying float vLight;\n"+
        "void main(){vec3 n=normalize((uModel*vec4(aNormal,0.0)).xyz);"+
        "vec3 light=normalize(vec3(-0.4,0.65,0.85));"+
        "vLight=clamp(0.68+0.34*max(dot(n,light),0.0),0.50,1.1);"+
        "vUV=aUV; gl_Position=uMVP*vec4(aPosition,1.0);}";
    private static final String FACE_FRAG=
        "precision mediump float; varying vec2 vUV; varying float vLight; uniform sampler2D uTex;"+
        "void main(){vec4 c=texture2D(uTex,vUV);gl_FragColor=vec4(c.rgb*vLight,1.0);}";
    private static final String HAIR_VERT=
        "attribute vec3 aPosition; uniform mat4 uMVP;"+
        "void main(){gl_Position=uMVP*vec4(aPosition,1.0);}";
    private static final String HAIR_FRAG=
        "precision mediump float; uniform vec4 uColor;"+
        "void main(){gl_FragColor=uColor;}";
    private static final String BG_VERT=
        "attribute vec2 aPosition; attribute vec2 aUV; varying vec2 vUV;"+
        "void main(){vUV=aUV;gl_Position=vec4(aPosition,0.99,1.0);}";
    private static final String BG_FRAG=
        "precision mediump float; varying vec2 vUV; uniform sampler2D uTex;"+
        "void main(){gl_FragColor=texture2D(uTex,vUV);}";

    public GameRenderer(Context ctx,GameState state){context=ctx;game=state;}
    private static FloatBuffer floats(int capacity){
        return ByteBuffer.allocateDirect(capacity*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
    }
    private static int shader(int type,String code){
        int sh=GLES20.glCreateShader(type);
        GLES20.glShaderSource(sh,code);GLES20.glCompileShader(sh);
        int[] ok=new int[1];GLES20.glGetShaderiv(sh,GLES20.GL_COMPILE_STATUS,ok,0);
        if(ok[0]==0){String error=GLES20.glGetShaderInfoLog(sh);GLES20.glDeleteShader(sh);throw new RuntimeException("Shader error: "+error);}
        return sh;
    }
    private static int program(String vs,String fs){
        int v=shader(GLES20.GL_VERTEX_SHADER,vs),f=shader(GLES20.GL_FRAGMENT_SHADER,fs);
        int p=GLES20.glCreateProgram();GLES20.glAttachShader(p,v);GLES20.glAttachShader(p,f);GLES20.glLinkProgram(p);
        int[] ok=new int[1];GLES20.glGetProgramiv(p,GLES20.GL_LINK_STATUS,ok,0);
        GLES20.glDeleteShader(v);GLES20.glDeleteShader(f);
        if(ok[0]==0)throw new RuntimeException("Program link error: "+GLES20.glGetProgramInfoLog(p));
        return p;
    }
    private static int texture(Bitmap image){
        int[] tmp=new int[1];GLES20.glGenTextures(1,tmp,0);
        int tex=tmp[0];GLES20.glBindTexture(GLES20.GL_TEXTURE_2D,tex);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_MIN_FILTER,GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_MAG_FILTER,GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_WRAP_S,GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_WRAP_T,GLES20.GL_CLAMP_TO_EDGE);
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D,0,image,0);
        image.recycle();return tex;
    }
    private static void oval(Canvas c,Paint p,float x,float y,float rx,float ry,int color){
        p.setShader(null);p.setColor(color);p.setStyle(Paint.Style.FILL);
        c.drawOval(new RectF(x-rx,y-ry,x+rx,y+ry),p);
    }
    private Bitmap createFace(){
        Bitmap b=Bitmap.createBitmap(256,256,Bitmap.Config.ARGB_8888);
        Canvas c=new Canvas(b);Paint p=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        int base=android.graphics.Color.rgb(222,177,145);
        p.setShader(new LinearGradient(0,0,256,240,new int[]{0xffc99877,0xffebc3a0,0xffdb9c7f},null,Shader.TileMode.CLAMP));
        c.drawRect(0,0,256,256,p);p.setShader(null);
        // Tonal side shadows are painted into the cylindrical face atlas.
        p.setShader(new LinearGradient(0,0,256,0,new int[]{0xffb88066,base,0xfff1c8a4,base,0xffb47f62},null,Shader.TileMode.CLAMP));
        c.drawRect(0,40,256,256,p);p.setShader(null);
        // Cheeks, brow sockets, eyelashes, blue irises and catchlights.
        oval(c,p,102,154,17,21,0x22b44a49);oval(c,p,153,154,17,21,0x22b44a49);
        for(int x:new int[]{111,145}){
            oval(c,p,x,109,16,8,0xff986f5b);
            oval(c,p,x,114,13,6,0xfff1ebe2);
            oval(c,p,x,114,6,6,0xff71a9b5);
            oval(c,p,x,114,3,4,0xff1b2f38);
            oval(c,p,x-2,112,2.2f,2.2f,0xffffffff);
            oval(c,p,x,106,14,2,0xff4b332b);
            oval(c,p,x,124,12,1.7f,0x886a4941);
            oval(c,p,x,96,16,3.3f,0xff563c31);
        }
        // Raised nose bridge, tiny nostrils, lips and chin shading.
        oval(c,p,128,149,5,22,0x14ffffff);
        oval(c,p,121,161,2.9f,1.8f,0xff8c5b4c);
        oval(c,p,135,161,2.9f,1.8f,0xff8c5b4c);
        oval(c,p,128,190,15,3.0f,0xffaa7063);
        oval(c,p,128,195,16,4.3f,0xffc48679);
        oval(c,p,128,193,14,1.0f,0xff683f3b);
        oval(c,p,128,218,17,3,0x22825244);
        p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(.7f);p.setColor(0x22815b47);
        for(int y=172;y<230;y+=6)for(int x=92;x<166;x+=8){
            if(((x*41+y*23)&7)==0 && Math.abs(x-128)<38)c.drawPoint(x,y,p);
        }
        p.setStyle(Paint.Style.FILL);
        return b;
    }
    private Bitmap createBackground(){
        Bitmap b=Bitmap.createBitmap(1024,512,Bitmap.Config.ARGB_8888);
        Canvas c=new Canvas(b);Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setShader(new LinearGradient(0,0,0,512,0xff101824,0xff304754,Shader.TileMode.CLAMP));
        c.drawRect(0,0,1024,512,p);p.setShader(null);
        p.setColor(0xff8a6941);c.drawRoundRect(new RectF(90,25,934,483),32,32,p);
        p.setColor(0xffc29b59);c.drawRoundRect(new RectF(98,34,926,475),24,24,p);
        p.setColor(0xff081a28);c.drawRoundRect(new RectF(110,45,914,462),17,17,p);
        p.setShader(new LinearGradient(110,50,910,460,new int[]{0xff284958,0xff142a39,0xff1d3745},null,Shader.TileMode.CLAMP));
        c.drawRoundRect(new RectF(117,52,906,454),15,15,p);p.setShader(null);
        p.setColor(0x335bd0e3);c.drawRect(170,58,186,450,p);
        p.setColor(0x2274c4cc);c.drawRect(820,60,831,447,p);
        p.setColor(0xffe2bd74);
        for(int x=180;x<910;x+=95)c.drawRoundRect(new RectF(x,31,x+34,40),5,5,p);
        p.setColor(0xff151a24);c.drawRect(0,476,1024,512,p);
        p.setColor(0xff8a693e);c.drawRect(0,476,1024,483,p);
        return b;
    }
    private void createHead(){
        final int rings=46,slices=72;
        faceCount=rings*slices*6;
        faceVertices=floats(faceCount*8);
        for(int i=0;i<rings;i++){
            float y0=1.14f-2.16f*i/rings;
            float y1=1.14f-2.16f*(i+1)/rings;
            for(int j=0;j<slices;j++){
                float a0=(j/(float)slices-.5f)*(float)(2*Math.PI);
                float a1=((j+1f)/slices-.5f)*(float)(2*Math.PI);
                addTri(y0,a0,y1,a0,y0,a1);
                addTri(y0,a1,y1,a0,y1,a1);
            }
        }
        faceVertices.position(0);
    }
    private void addTri(float y0,float a0,float y1,float a1,float y2,float a2){
        vertex(y0,a0);vertex(y1,a1);vertex(y2,a2);
    }
    private void vertex(float y,float angle){
        float[] pt=GameState.headPoint(y,angle);
        float nx=(float)Math.sin(angle),nz=(float)Math.cos(angle),ny=y*.45f;
        float inv=(float)(1/Math.sqrt(nx*nx+ny*ny+nz*nz));
        faceVertices.put(pt[0]).put(pt[1]).put(pt[2]);
        faceVertices.put((angle/(float)(2*Math.PI))+.5f).put((1.14f-y)/2.16f);
        faceVertices.put(nx*inv).put(ny*inv).put(nz*inv);
    }
    private void updateHair(){
        int remaining=0;
        for(GameState.Hair h:game.hairs)if(!h.gone)remaining++;
        hairCount=remaining*2;
        int count=Math.max(1,hairCount*3);
        if(hairVertices==null||hairVertices.capacity()<count)hairVertices=floats(count);
        hairVertices.clear();
        for(GameState.Hair h:game.hairs){
            if(h.gone)continue;
            hairVertices.put(h.x).put(h.y).put(h.z);
            hairVertices.put(h.x+h.nx*h.len*.8f).put(h.y+h.len*.75f).put(h.z+h.nz*h.len*.8f);
        }
        hairVertices.flip();
        game.hairDirty=false;
    }
    @Override public void onSurfaceCreated(GL10 unused,EGLConfig config){
        GLES20.glClearColor(.06f,.12f,.17f,1);
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDepthFunc(GLES20.GL_LEQUAL);
        GLES20.glDisable(GLES20.GL_CULL_FACE);
        faceProgram=program(FACE_VERT,FACE_FRAG);
        hairProgram=program(HAIR_VERT,HAIR_FRAG);
        backgroundProgram=program(BG_VERT,BG_FRAG);
        faceTexture=texture(createFace());
        backgroundTexture=texture(createBackground());
        backgroundVertices=floats(16);
        backgroundVertices.put(new float[]{-1,-1,0,0, 1,-1,1,0, -1,1,0,1, 1,1,1,1});
        backgroundVertices.position(0);
        createHead();
        synchronized(game){game.hairDirty=true;}
        lastTime=System.nanoTime();
    }
    @Override public void onSurfaceChanged(GL10 unused,int width,int height){
        surfaceWidth=width;surfaceHeight=height;
        GLES20.glViewport(0,0,width,height);
        Matrix.perspectiveM(projection,0,50.0f,width/(float)Math.max(1,height),.1f,50f);
    }
    private void attrib(int program,String name,int size,int stride,FloatBuffer buf,int start){
        int loc=GLES20.glGetAttribLocation(program,name);
        if(loc<0)return;
        buf.position(start);GLES20.glEnableVertexAttribArray(loc);
        GLES20.glVertexAttribPointer(loc,size,GLES20.GL_FLOAT,false,stride,buf);
    }
    private void unattrib(int program,String... names){
        for(String name:names){
            int loc=GLES20.glGetAttribLocation(program,name);
            if(loc>=0)GLES20.glDisableVertexAttribArray(loc);
        }
    }
    private void drawBackground(){
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glUseProgram(backgroundProgram);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D,backgroundTexture);
        GLES20.glUniform1i(GLES20.glGetUniformLocation(backgroundProgram,"uTex"),0);
        attrib(backgroundProgram,"aPosition",2,16,backgroundVertices,0);
        attrib(backgroundProgram,"aUV",2,16,backgroundVertices,2);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP,0,4);
        unattrib(backgroundProgram,"aPosition","aUV");
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
    }
    @Override public void onDrawFrame(GL10 unused){
        long now=System.nanoTime();
        float dt=Math.min(.1f,(now-lastTime)*1e-9f);
        lastTime=now;
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT|GLES20.GL_DEPTH_BUFFER_BIT);
        drawBackground();
        synchronized(game){
            game.update(dt);
            if(game.hairDirty||lastCandidate!=game.candidate){
                updateHair();lastCandidate=game.candidate;
            }
            Matrix.setLookAtM(view,0,0,0,game.distance(),0,.05f,0,0,1,0);
            Matrix.setIdentityM(model,0);
            Matrix.rotateM(model,0,(float)Math.toDegrees(game.pitch),1,0,0);
            Matrix.rotateM(model,0,(float)Math.toDegrees(game.yaw),0,1,0);
            Matrix.multiplyMM(mv,0,view,0,model,0);
            Matrix.multiplyMM(mvp,0,projection,0,mv,0);
            GLES20.glUseProgram(faceProgram);
            GLES20.glUniformMatrix4fv(GLES20.glGetUniformLocation(faceProgram,"uMVP"),1,false,mvp,0);
            GLES20.glUniformMatrix4fv(GLES20.glGetUniformLocation(faceProgram,"uModel"),1,false,model,0);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D,faceTexture);
            GLES20.glUniform1i(GLES20.glGetUniformLocation(faceProgram,"uTex"),0);
            attrib(faceProgram,"aPosition",3,32,faceVertices,0);
            attrib(faceProgram,"aUV",2,32,faceVertices,3);
            attrib(faceProgram,"aNormal",3,32,faceVertices,5);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLES,0,faceCount);
            unattrib(faceProgram,"aPosition","aUV","aNormal");
            if(hairCount>0){
                GLES20.glUseProgram(hairProgram);
                GLES20.glUniformMatrix4fv(GLES20.glGetUniformLocation(hairProgram,"uMVP"),1,false,mvp,0);
                float[][] colors={{.24f,.15f,.10f,1f},{.90f,.18f,.44f,1f},{.85f,.81f,.70f,1f}};
                GLES20.glUniform4fv(GLES20.glGetUniformLocation(hairProgram,"uColor"),1,colors[game.candidate],0);
                GLES20.glLineWidth(game.candidate==1?2.2f:1.7f);
                attrib(hairProgram,"aPosition",3,12,hairVertices,0);
                GLES20.glDrawArrays(GLES20.GL_LINES,0,hairCount);
                unattrib(hairProgram,"aPosition");
            }
        }
    }
}
