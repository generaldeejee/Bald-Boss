package com.baldelite.game;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Random;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

/** Bald Elite Deluxe: shaded HD procedural head, sculpted facial geometry,
 * multisample-ready OpenGL ES 2, per-hair ribbon triangles and full-size
 * original club background. All geometry is static until quality changes.
 */
public final class GameRenderer implements GLSurfaceView.Renderer {
    private final GameState game;
    private final Context context;
    private int surfaceWidth=960,surfaceHeight=540;
    private int faceProgram,hairProgram,backgroundProgram,faceTexture,backgroundTexture;
    private FloatBuffer faceVertices,hairVertices,backgroundVertices;
    private int faceCount=0,hairCount=0,lastCandidate=-1;
    private boolean lastQuality=false;
    private final float[] projection=new float[16],view=new float[16],model=new float[16],mv=new float[16],mvp=new float[16];
    private long lastTime=0;
    private static final String FACE_VERT=
        "attribute vec3 aPosition;attribute vec2 aUV;attribute vec3 aNormal;"+
        "uniform mat4 uMVP;uniform mat4 uModel;varying vec2 vUV;varying float vLight;varying vec3 vNormal;"+
        "void main(){vec3 n=normalize((uModel*vec4(aNormal,0.0)).xyz);"+
        "vec3 key=normalize(vec3(-0.50,0.72,0.86));"+
        "vec3 fill=normalize(vec3(0.65,0.25,0.55));"+
        "vLight=0.40+0.42*max(dot(n,key),0.0)+0.19*max(dot(n,fill),0.0);"+
        "vNormal=n;vUV=aUV;gl_Position=uMVP*vec4(aPosition,1.0);}";
    private static final String FACE_FRAG=
        "precision mediump float;varying vec2 vUV;varying float vLight;varying vec3 vNormal;"+
        "uniform sampler2D uTex;uniform vec3 uTint;"+
        "void main(){vec3 n=normalize(vNormal);vec3 key=normalize(vec3(-0.50,0.72,0.86));"+
        "vec3 r=reflect(-key,n);vec3 eye=normalize(vec3(0.0,0.12,1.0));"+
        "float shine=pow(max(dot(r,eye),0.0),28.0)*0.30;"+
        "float rim=pow(1.0-max(dot(n,eye),0.0),3.0)*0.17;"+
        "vec3 base=texture2D(uTex,vUV).rgb*uTint;"+
        "vec3 result=base*vLight+vec3(1.0,0.82,0.56)*shine+vec3(0.40,0.25,0.70)*rim;"+
        "gl_FragColor=vec4(clamp(result,0.0,1.0),1.0);}";
    private static final String HAIR_VERT=
        "attribute vec3 aPosition;attribute vec3 aColor;uniform mat4 uMVP;varying vec3 vColor;"+
        "void main(){vColor=aColor;gl_Position=uMVP*vec4(aPosition,1.0);}";
    private static final String HAIR_FRAG=
        "precision mediump float;varying vec3 vColor;"+
        "void main(){gl_FragColor=vec4(vColor,1.0);}";
    private static final String BG_VERT=
        "attribute vec2 aPosition;attribute vec2 aUV;varying vec2 vUV;"+
        "void main(){vUV=aUV;gl_Position=vec4(aPosition,0.99,1.0);}";
    private static final String BG_FRAG=
        "precision mediump float;varying vec2 vUV;uniform sampler2D uTex;"+
        "void main(){gl_FragColor=texture2D(uTex,vUV);}";

    public GameRenderer(Context ctx,GameState state){context=ctx;game=state;}
    private static FloatBuffer floats(int capacity){
        return ByteBuffer.allocateDirect(capacity*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
    }
    private static int shader(int type,String code){
        int sh=GLES20.glCreateShader(type);
        GLES20.glShaderSource(sh,code);GLES20.glCompileShader(sh);
        int[] ok=new int[1];GLES20.glGetShaderiv(sh,GLES20.GL_COMPILE_STATUS,ok,0);
        if(ok[0]==0){String error=GLES20.glGetShaderInfoLog(sh);GLES20.glDeleteShader(sh);throw new RuntimeException("Shader: "+error);}
        return sh;
    }
    private static int program(String vs,String fs){
        int v=shader(GLES20.GL_VERTEX_SHADER,vs),f=shader(GLES20.GL_FRAGMENT_SHADER,fs);
        int p=GLES20.glCreateProgram();GLES20.glAttachShader(p,v);GLES20.glAttachShader(p,f);GLES20.glLinkProgram(p);
        int[] ok=new int[1];GLES20.glGetProgramiv(p,GLES20.GL_LINK_STATUS,ok,0);
        GLES20.glDeleteShader(v);GLES20.glDeleteShader(f);
        if(ok[0]==0)throw new RuntimeException("Program: "+GLES20.glGetProgramInfoLog(p));
        return p;
    }
    private static int texture(Bitmap image){
        int[] temp=new int[1];GLES20.glGenTextures(1,temp,0);
        int tex=temp[0];GLES20.glBindTexture(GLES20.GL_TEXTURE_2D,tex);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_MIN_FILTER,GLES20.GL_LINEAR_MIPMAP_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_MAG_FILTER,GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_WRAP_S,GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,GLES20.GL_TEXTURE_WRAP_T,GLES20.GL_CLAMP_TO_EDGE);
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D,0,image,0);
        GLES20.glGenerateMipmap(GLES20.GL_TEXTURE_2D);
        image.recycle();return tex;
    }
    private static void oval(Canvas c,Paint p,float x,float y,float rx,float ry,int color){
        p.setShader(null);p.setColor(color);p.setStyle(Paint.Style.FILL);
        c.drawOval(new RectF(x-rx,y-ry,x+rx,y+ry),p);
    }
    private Bitmap createFace(){
        int resolution=game.highDetail?1024:512;
        Bitmap b=Bitmap.createBitmap(resolution,resolution,Bitmap.Config.ARGB_8888);
        Canvas c=new Canvas(b);
        c.scale(resolution/256f,resolution/256f);
        Paint p=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        p.setShader(new LinearGradient(0,0,256,240,
            new int[]{0xffbd896f,0xffe7b99b,0xffddae8b,0xffca8f71},null,Shader.TileMode.CLAMP));
        c.drawRect(0,0,256,256,p);
        p.setShader(new LinearGradient(0,0,256,0,
            new int[]{0xff9e6754,0xffd6a284,0xffebbea0,0xffe9bba0,0xffb3785e},null,Shader.TileMode.CLAMP));
        c.drawRect(0,35,256,256,p);p.setShader(null);
        // Temporal brow/temple, rosy cheeks, forehead glints and skin shading.
        oval(c,p,92,128,20,33,0x1a734333);oval(c,p,164,128,20,33,0x1a734333);
        oval(c,p,103,149,24,20,0x30d1807b);oval(c,p,153,149,24,20,0x30d1807b);
        oval(c,p,128,46,55,29,0x18fff0d1);
        oval(c,p,127,177,20,32,0x1b874737);
        for(int x:new int[]{105,151}){
            // Detailed almond-shaped eyelids, iris, pupil, highlight and lower socket.
            oval(c,p,x,106,21,4.5f,0xff644b40);
            oval(c,p,x,114,15.8f,7,0xff66473b);
            oval(c,p,x,114,15,6.4f,0xfff0e4d7);
            oval(c,p,x,114,7.5f,6.6f,0xff518e9b);
            oval(c,p,x,114,5.4f,5,0xff377384);
            oval(c,p,x,114,3.4f,4.4f,0xff27323e);
            oval(c,p,x-2.6f,111.8f,1.9f,2.0f,0xffffffff);
            oval(c,p,x+3.2f,116.4f,1.0f,1.0f,0xffc5e8e1);
            oval(c,p,x,121,14,1.3f,0x885f4640);
            oval(c,p,x,98,20,3.7f,0xff68483c);
            oval(c,p,x,96,18,1.4f,0xff99745a);
        }
        // Nose bridge, tip, nostrils, philtrum, mouth and natural lips.
        oval(c,p,128,141,6.4f,25,0x15ffffff);
        oval(c,p,128,165,12,7,0x1e7c4435);
        oval(c,p,118.9f,164,3.2f,2.4f,0xff7e5144);
        oval(c,p,137.1f,164,3.2f,2.4f,0xff7e5144);
        oval(c,p,128,173,2.5f,5.5f,0x337f5147);
        oval(c,p,128,190,19,3.4f,0xffaa766c);
        oval(c,p,128,194,17,5,0xffca9382);
        oval(c,p,128,192.5f,15,1.0f,0xff78483f);
        oval(c,p,128,221,19,4,0x23734432);
        // Pores and freckles are deterministic, subtle and zoom-safe.
        Random rand=new Random(0xB01D2026L);
        for(int k=0;k<(game.highDetail?1400:550);k++){
            float x=45+rand.nextFloat()*166;
            float y=39+rand.nextFloat()*209;
            int a=(k%7==0)?24:11;
            int tint=(a<<24)|(76<<16)|(45<<8)|34;
            p.setColor(tint);
            c.drawCircle(x,y,(k%11==0?.23f:.13f),p);
        }
        // Very fine sheen on the scalp, rendered as texture detail and GPU light.
        p.setColor(0x29fff1d3);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(.45f);
        c.drawArc(new RectF(85,31,168,75),208,113,false,p);
        p.setStyle(Paint.Style.FILL);
        return b;
    }
    private Bitmap createBackground(){
        try(InputStream stream=context.getAssets().open("club_menu.jpg")){
            Bitmap img=BitmapFactory.decodeStream(stream);
            if(img!=null)return img;
        }catch(Exception ignored){}
        Bitmap b=Bitmap.createBitmap(1024,512,Bitmap.Config.ARGB_8888);
        Canvas c=new Canvas(b);Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setShader(new LinearGradient(0,0,0,512,0xff101824,0xff304754,Shader.TileMode.CLAMP));
        c.drawRect(0,0,1024,512,p);p.setShader(null);
        p.setColor(0xff8a6941);c.drawRoundRect(new RectF(90,25,934,483),32,32,p);
        p.setColor(0xffc29b59);c.drawRoundRect(new RectF(98,34,926,475),24,24,p);
        p.setColor(0xff081a28);c.drawRoundRect(new RectF(110,45,914,462),17,17,p);
        p.setShader(new LinearGradient(110,50,910,460,
            new int[]{0xff284958,0xff142a39,0xff1d3745},null,Shader.TileMode.CLAMP));
        c.drawRoundRect(new RectF(117,52,906,454),15,15,p);p.setShader(null);
        p.setColor(0xff151a24);c.drawRect(0,476,1024,512,p);
        return b;
    }
    private void faceVertex(float y,float angle){
        float[] pt=GameState.headPoint(y,angle);
        float nx=(float)Math.sin(angle),nz=(float)Math.cos(angle),ny=y*.36f;
        float inv=(float)(1/Math.sqrt(nx*nx+ny*ny+nz*nz));
        faceVertices.put(pt[0]).put(pt[1]).put(pt[2]);
        faceVertices.put((angle/(float)(2*Math.PI))+.5f).put((1.14f-y)/2.16f);
        faceVertices.put(nx*inv).put(ny*inv).put(nz*inv);
    }
    private void tri(float y0,float a0,float y1,float a1,float y2,float a2){
        faceVertex(y0,a0);faceVertex(y1,a1);faceVertex(y2,a2);
    }
    private void neckVertex(float y,float angle){
        float sn=(float)Math.sin(angle),cs=(float)Math.cos(angle);
        float r=.225f+(.04f*Math.max(0,(-y-1.1f)));
        float x=r*sn,z=-.07f+.19f*cs;
        faceVertices.put(x).put(y).put(z);
        faceVertices.put(.5f+sn*.05f).put(.89f);
        faceVertices.put(sn).put(0).put(cs);
    }
    private void neckTri(float y0,float a0,float y1,float a1,float y2,float a2){
        neckVertex(y0,a0);neckVertex(y1,a1);neckVertex(y2,a2);
    }
    private void createHead(){
        final int rings=game.highDetail?86:48, slices=game.highDetail?120:72;
        final int neckRings=8,neckSlices=game.highDetail?48:32;
        faceCount=rings*slices*6+neckRings*neckSlices*6;
        faceVertices=floats(faceCount*8);
        for(int i=0;i<rings;i++){
            float y0=1.14f-2.16f*i/rings,y1=1.14f-2.16f*(i+1)/rings;
            for(int j=0;j<slices;j++){
                float a0=(j/(float)slices-.5f)*(float)(2*Math.PI);
                float a1=((j+1f)/slices-.5f)*(float)(2*Math.PI);
                tri(y0,a0,y1,a0,y0,a1);tri(y0,a1,y1,a0,y1,a1);
            }
        }
        for(int i=0;i<neckRings;i++){
            float y0=-1.01f-.37f*i/neckRings;
            float y1=-1.01f-.37f*(i+1)/neckRings;
            for(int j=0;j<neckSlices;j++){
                float a0=j/(float)neckSlices*(float)(2*Math.PI);
                float a1=(j+1f)/neckSlices*(float)(2*Math.PI);
                neckTri(y0,a0,y1,a0,y0,a1);
                neckTri(y0,a1,y1,a0,y1,a1);
            }
        }
        faceVertices.position(0);
    }
    private void addRibbon(GameState.Hair hair,int strand,FloatBuffer out){
        // A tapering, triangular hair ribbon with a subtle highlight at the tip.
        float[] rgb=game.candidate==0?new float[]{.25f,.15f,.105f}:
                    game.candidate==1?new float[]{.90f,.20f,.47f}:
                    new float[]{.76f,.74f,.68f};
        float bright=(strand%9)*.025f;
        float tx=hair.nz,tz=-hair.nx;
        float width=game.candidate==1?.0095f:.006f;
        float rootX=hair.x+hair.nx*.01f,rootY=hair.y+hair.ny*.01f,rootZ=hair.z+hair.nz*.01f;
        float tipX=rootX+hair.nx*hair.len*.78f;
        float tipY=rootY+hair.len*.72f;
        float tipZ=rootZ+hair.nz*hair.len*.78f;
        ribbonVertex(out,rootX-tx*width,rootY,rootZ-tz*width,rgb,bright);
        ribbonVertex(out,rootX+tx*width,rootY,rootZ+tz*width,rgb,bright);
        ribbonVertex(out,tipX,tipY,tipZ,rgb,bright+.16f);
    }
    private static void ribbonVertex(FloatBuffer out,float x,float y,float z,float[] rgb,float bright){
        out.put(x).put(y).put(z);
        out.put(Math.min(1f,rgb[0]+bright)).put(Math.min(1f,rgb[1]+bright*.7f)).put(Math.min(1f,rgb[2]+bright*.5f));
    }
    private void updateHair(){
        int remain=0;
        for(GameState.Hair h:game.hairs)if(!h.gone)remain++;
        hairCount=remain*3;
        int count=Math.max(6,hairCount*6);
        if(hairVertices==null||hairVertices.capacity()<count)hairVertices=floats(count);
        hairVertices.clear();
        for(int i=0;i<game.hairs.length;i++){
            GameState.Hair h=game.hairs[i];
            if(!h.gone)addRibbon(h,i,hairVertices);
        }
        hairVertices.flip();game.hairDirty=false;
    }
    @Override public void onSurfaceCreated(GL10 unused,EGLConfig config){
        GLES20.glClearColor(.04f,.07f,.13f,1f);
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDepthFunc(GLES20.GL_LEQUAL);
        GLES20.glDisable(GLES20.GL_CULL_FACE);
        faceProgram=program(FACE_VERT,FACE_FRAG);
        hairProgram=program(HAIR_VERT,HAIR_FRAG);
        backgroundProgram=program(BG_VERT,BG_FRAG);
        faceTexture=texture(createFace());
        backgroundTexture=texture(createBackground());
        backgroundVertices=floats(16);
        // UV coordinates flipped for Android Bitmap's top-left origin.
        backgroundVertices.put(new float[]{-1,-1,0,1, 1,-1,1,1, -1,1,0,0, 1,1,1,0});
        backgroundVertices.position(0);
        createHead();
        synchronized(game){game.hairDirty=true;lastQuality=game.highDetail;}
        lastTime=System.nanoTime();
    }
    @Override public void onSurfaceChanged(GL10 unused,int w,int h){
        surfaceWidth=w;surfaceHeight=h;
        GLES20.glViewport(0,0,w,h);
        Matrix.perspectiveM(projection,0,50f,w/(float)Math.max(1,h),.1f,50f);
    }
    private static void attrib(int program,String name,int size,int stride,FloatBuffer buf,int start){
        int loc=GLES20.glGetAttribLocation(program,name);
        if(loc<0)return;
        buf.position(start);GLES20.glEnableVertexAttribArray(loc);
        GLES20.glVertexAttribPointer(loc,size,GLES20.GL_FLOAT,false,stride,buf);
    }
    private static void unattrib(int program,String... names){
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
            if(game.highDetail!=lastQuality){
                createHead();
                int[] previous={faceTexture};
                GLES20.glDeleteTextures(1,previous,0);
                faceTexture=texture(createFace());
                lastQuality=game.highDetail;
            }
            if(game.hairDirty||lastCandidate!=game.candidate){
                updateHair();lastCandidate=game.candidate;
            }
            if(game.phase==GameState.INTRO||game.phase==GameState.WIN||game.phase==GameState.FAIL)return;
            Matrix.setLookAtM(view,0,0,0,game.distance(),0,.05f,0,0,1,0);
            Matrix.setIdentityM(model,0);
            Matrix.rotateM(model,0,(float)Math.toDegrees(game.pitch),1,0,0);
            Matrix.rotateM(model,0,(float)Math.toDegrees(game.yaw),0,1,0);
            Matrix.multiplyMM(mv,0,view,0,model,0);
            Matrix.multiplyMM(mvp,0,projection,0,mv,0);
            GLES20.glUseProgram(faceProgram);
            GLES20.glUniformMatrix4fv(GLES20.glGetUniformLocation(faceProgram,"uMVP"),1,false,mvp,0);
            GLES20.glUniformMatrix4fv(GLES20.glGetUniformLocation(faceProgram,"uModel"),1,false,model,0);
            float[] skin=game.candidate==1?new float[]{1.05f,.92f,.95f}:
                         game.candidate==2?new float[]{.97f,.99f,1.04f}:new float[]{1f,1f,1f};
            GLES20.glUniform3fv(GLES20.glGetUniformLocation(faceProgram,"uTint"),1,skin,0);
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
                attrib(hairProgram,"aPosition",3,24,hairVertices,0);
                attrib(hairProgram,"aColor",3,24,hairVertices,3);
                GLES20.glDrawArrays(GLES20.GL_TRIANGLES,0,hairCount);
                unattrib(hairProgram,"aPosition","aColor");
            }
        }
    }
}
