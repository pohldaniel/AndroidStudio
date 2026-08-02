package com.android.webgpu;

import androidx.appcompat.app.AppCompatActivity;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.widget.Button;

import android.widget.FrameLayout;
import androidx.appcompat.widget.Toolbar;

public class MainActivity extends AppCompatActivity {
    private final String[] appStates = {"Collada", "Wireframe", "Deferred", "Particle", "Volume", "Isometric"};
    private static int CurrentStateIndex = 0;
    private View view;

    //will trigger by rotate as well
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        setContentView(R.layout.activity_main);
        FrameLayout webgpuContainer = findViewById(R.id.webgpu_container);

        view = new View(this);
        view.initRenderer(getAssets());
        webgpuContainer.addView(view);

        Toolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitle(appStates[CurrentStateIndex]);
        setSupportActionBar(toolbar);

        Button buttonLeft = findViewById(R.id.button_left);
        Button buttonAction = findViewById(R.id.button_center);
        Button buttonRight = findViewById(R.id.button_right);

        if (buttonLeft != null && buttonRight != null && buttonAction != null) {
            int length = appStates.length - 1;
            buttonAction.setVisibility(View.GONE);
            buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);

            buttonLeft.setOnClickListener(v -> {
                if (CurrentStateIndex > 0) {
                    CurrentStateIndex--;
                }else{
                    CurrentStateIndex = length;
                }
                toolbar.setTitle(appStates[CurrentStateIndex]);
                NativeLibrary.OnButton(0);
                buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);
            });

            buttonRight.setOnClickListener(v -> {
                if (CurrentStateIndex < length) {
                    CurrentStateIndex++;
                }else{
                    CurrentStateIndex = 0;
                }

                toolbar.setTitle(appStates[CurrentStateIndex]);
                NativeLibrary.OnButton(2);
                buttonAction.setVisibility(CurrentStateIndex == 1 ? View.VISIBLE : View.GONE);
            });

            buttonAction.setOnClickListener(
                v-> NativeLibrary.OnButton(1)
            );
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        view.onDestroy();
    }
}

