package com.android.webgpu;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    AppView appView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        appView = new AppView(this);
        appView.initRenderer(getAssets());
        setContentView(appView);
    }

    @Override
    protected void onPause() {
        super.onPause();
        appView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        appView.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}

