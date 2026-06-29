package com.android.webgpu;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;


public class MainActivity extends AppCompatActivity {
    private final String[] appStates = {"Zustand 1: Init", "Zustand 2: WebGPU Pipeline", "Zustand 3: Render Loop"};
    private int currentStateIndex = 0;

    private TextView statusText;
    private Button buttonLeft;
    private Button buttonRight;

    View view;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setContentView(R.layout.activity_main);
        FrameLayout container = findViewById(R.id.webgpu_container);

        view = new View(this);
        view.initRenderer(getAssets());

        container.addView(view);

        statusText = findViewById(R.id.textViewState);
        buttonLeft = findViewById(R.id.button_left);
        buttonRight = findViewById(R.id.button_right);

        if (buttonLeft != null) {
            buttonLeft.setOnClickListener(v -> {
                if (currentStateIndex > 0) {
                    currentStateIndex--;
                    statusText.setText(appStates[currentStateIndex]);
                }
            });
        }

        if (buttonRight != null) {
            buttonRight.setOnClickListener(v -> {
                if (currentStateIndex < appStates.length - 1) {
                    currentStateIndex++;
                    statusText.setText(appStates[currentStateIndex]);
                }
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

