package com.android.webgpu;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.widget.Button;

import android.widget.FrameLayout;
import androidx.appcompat.widget.Toolbar;

public class MainActivity extends AppCompatActivity {
    private final String[] appStates = {"Collada", "Wireframe", "Deferred", "Particle", "Volume"};
    private int currentStateIndex = 0;
    private Toolbar toolbar;
    View view;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);
        FrameLayout container = findViewById(R.id.webgpu_container);

        view = new View(this);
        view.initRenderer(getAssets());
        container.addView(view);
        toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        Button buttonLeft = findViewById(R.id.button_left);
        Button buttonAction = findViewById(R.id.button_center);
        Button buttonRight = findViewById(R.id.button_right);

        if (buttonLeft != null && buttonRight != null && buttonAction != null) {
            int length = appStates.length - 1;
            buttonAction.setVisibility(View.GONE);

            buttonLeft.setOnClickListener(v -> {
                if (currentStateIndex > 0) {
                    currentStateIndex--;
                    toolbar.setTitle(appStates[currentStateIndex]);
                }else{
                    currentStateIndex = length;
                    toolbar.setTitle(appStates[currentStateIndex]);
                }

                NativeLibrary.OnButton(0);
                buttonAction.setVisibility(currentStateIndex == 1 ? View.VISIBLE : View.GONE);
            });

            buttonRight.setOnClickListener(v -> {
                if (currentStateIndex < length) {
                    currentStateIndex++;
                    toolbar.setTitle(appStates[currentStateIndex]);
                }else{
                    currentStateIndex = 0;
                    toolbar.setTitle(appStates[currentStateIndex]);
                }

                NativeLibrary.OnButton(2);
                buttonAction.setVisibility(currentStateIndex == 1 ? View.VISIBLE : View.GONE);
            });

            buttonAction.setOnClickListener(v -> {
                NativeLibrary.OnButton(1);
            });
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

