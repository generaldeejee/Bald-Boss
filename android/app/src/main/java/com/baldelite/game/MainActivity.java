package com.baldelite.game;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;

public final class MainActivity extends Activity {
    private GameState game;
    private GLSurfaceView gl;
    private SoundEngine sound;
    private void immersive(){
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN|
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE|
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
    }
    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        immersive();
        game=new GameState(getSharedPreferences("bald_elite_scores",MODE_PRIVATE));
        gl=new GLSurfaceView(this);
        gl.setEGLContextClientVersion(2);
        gl.setEGLConfigChooser(8,8,8,8,16,0);
        gl.setPreserveEGLContextOnPause(true);
        gl.setRenderer(new GameRenderer(this,game));
        gl.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        FrameLayout root=new FrameLayout(this);
        root.addView(gl,new FrameLayout.LayoutParams(-1,-1));
        root.addView(new HudView(this,game),new FrameLayout.LayoutParams(-1,-1));
        setContentView(root);
        sound=new SoundEngine(game);
        sound.start();
    }
    @Override protected void onResume(){
        super.onResume();
        immersive();
        if(gl!=null)gl.onResume();
        if(sound!=null)sound.start();
    }
    @Override protected void onPause(){
        if(sound!=null)sound.stop();
        if(gl!=null)gl.onPause();
        super.onPause();
    }
    @Override protected void onDestroy(){
        if(sound!=null)sound.stop();
        super.onDestroy();
    }
    @Override public void onBackPressed(){
        synchronized(game){
            if(game.phase==GameState.TITLE){super.onBackPressed();}
            else game.menu();
        }
    }
}
