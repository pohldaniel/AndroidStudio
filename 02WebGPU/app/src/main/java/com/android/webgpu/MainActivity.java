package com.android.webgpu;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.widget.Button;

import android.widget.FrameLayout;
import androidx.appcompat.widget.Toolbar;

public class MainActivity extends AppCompatActivity {
    private final String[] appStates = {"Collada", "Wireframe"};
    private int currentStateIndex = 0;
    private Toolbar toolbar;
    View view;
    private static AssetManager assetManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setContentView(R.layout.activity_main);
        FrameLayout container = findViewById(R.id.webgpu_container);
        AssetManager assetManager = getAssets();

        view = new View(this);
        view.initRenderer(assetManager);
        container.addView(view);
        toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        Button buttonLeft = findViewById(R.id.button_left);
        Button buttonAction = findViewById(R.id.button_center);
        Button buttonRight = findViewById(R.id.button_right);

        if (buttonLeft != null && buttonRight != null && buttonAction != null) {
            buttonAction.setVisibility(View.GONE);
            buttonLeft.setEnabled(false);
            buttonLeft.setOnClickListener(v -> {
                int prevStateIndex = currentStateIndex;
                if (currentStateIndex > 0) {
                    currentStateIndex--;
                    toolbar.setTitle(appStates[currentStateIndex]);
                    buttonRight.setEnabled(true);
                }

                if(prevStateIndex != currentStateIndex){
                    view.onButton();
                }

                if(currentStateIndex == 0) {
                    buttonLeft.setEnabled(false);
                    buttonAction.setVisibility(View.GONE);
                }
            });

            buttonRight.setOnClickListener(v -> {
                int prevStateIndex = currentStateIndex;
                if (currentStateIndex < 1) {
                    currentStateIndex++;
                    toolbar.setTitle(appStates[currentStateIndex]);
                    buttonLeft.setEnabled(true);
                }

                if(prevStateIndex != currentStateIndex){
                    view.onButton();
                }
                if(currentStateIndex == 1) {
                    buttonRight.setEnabled(false);
                    buttonAction.setVisibility(View.VISIBLE);
                }

            });

            buttonAction.setOnClickListener(v -> {
                view.onAction();
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

