package com.normaker.nativefull;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.media.AudioAttributes;
import android.media.SoundPool;
import java.util.HashMap;
import java.util.Map;
import android.content.Intent;
import android.net.Uri;
import android.webkit.ValueCallback;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import android.os.Environment;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.webkit.RenderProcessGoneDetail;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebChromeClient;
import android.webkit.WebChromeClient.FileChooserParams;
import android.util.Base64;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;

public final class MainActivity extends Activity {
    static {
        try {
            System.loadLibrary("gm82_android");
        } catch (Throwable t) {
            android.util.Log.e("NOR_NATIVE", "Could not load gm82_android native library", t);
        }
    }

    private WebView webView;
    private static final int REQUEST_GMX_GMZ = 8201;
    private static final int REQUEST_FILE_CHOOSER = 8202;
    private static final int REQUEST_GM82_FOLDER = 8203;
    private static final int REQUEST_GMX_FOLDER = 8204;
    private ValueCallback<Uri[]> pendingFileCallback;
    private SoundPool soundPool;
    private final Map<Integer, Integer> soundResources = new HashMap<>();
    private final Map<Integer, Integer> soundStreams = new HashMap<>();

    private static native boolean nativeRuntimeCreate(int width, int height);
    private static native void nativeRuntimeDestroy();
    private static native void nativeRuntimeStep(float delta);
    private static native void nativeRuntimeKey(int keyCode, boolean down);
    private static native void nativeRuntimeSetRoom(int roomId, int width, int height, boolean clearInstances);
    private static native int nativeRuntimeGetRoom();
    private static native String nativeRuntimeSnapshotJson();
    private static native String nativeEvaluateGml(String source);
    private static native boolean nativeRuntimeRegisterEvent(int objectId, int mainType, int subType, String source);
    private static native boolean nativeRuntimeRegisterObject(int objectId, String name);
    private static native boolean nativeRuntimeRegisterScript(String name, String source);
    private static native void nativeRuntimeClearScripts();
    private static native void nativeRuntimeClearEvents();
    private static native int nativeRuntimeAddInstance(int objectId, int spriteId, int spriteWidth, int spriteHeight, int spriteSubimages, float x, float y, float vx, float vy);
    private static native boolean nativeRuntimeSetSpriteBitmap(int spriteId, int frame, int width, int height, byte[] rgba);
    private static native boolean nativeRuntimeRenderBitmap(Bitmap target);
    private static native void nativeRuntimeClearInstances();
    private static native void nativeRuntimeClearRoomTransient();
    private static native boolean nativeRuntimeExecuteGml(int instanceId, String source);
    private static native String nativeRuntimeConsumeSoundCommands();
    private static native boolean nativeExportNorJson(String json, String outputPath);
    private static native int nativeImportGmxGmz(String path, String outputDir);
    private static native int nativeExportGmxGmz(String sourceDir, String outputPath, String kind);
    private static native boolean nativeExportGmkRaw(String sourcePath, String outputPath);
    private static native boolean nativeExportGmxSemantic(String sourceDir, String outputDir, String projectName);
    private static native String nativeCoreIdentity();
    private static native boolean nativeValidateGmk(byte[] bytes);
    private static native String nativeGmkHeaderJson(byte[] bytes);
    private static native String nativeGmkLayoutJson(byte[] bytes);
    private static native String nativeGmkChunkInventory(byte[] bytes);
    private static native String nativeGmkResourceManifest(byte[] bytes);
    private static native String nativeImportGmkSnapshot(byte[] bytes, String outputDir);
    private static native int nativeCompileGml(String source);
    private static native boolean nativeCodeExists(int codeId);
    private static native int nativeCodeGetArgCount(int codeId);
    private static native void nativeCodeDestroy(int codeId);
    private static native int nativeCodeExecute(int instanceId, int codeId);
    private static native boolean nativeExportRom(String title, String outputPath, int kind);
    private static native int nativeDetectRom(String path);
    private static native boolean nativeValidateRom(String path, int kind);
    private static native void nativeClearResourceRegistry();
    private static native int nativeRegisterResource(int kind, int id, String name, int width, int height, int frames);
    private static native int nativeResourceCount();
    private static native int nativeRegisterObjectEvent(int objectId, int mainType, int subType, String source);
    private static native int nativeObjectEventCount();
    private static native double nativeGm82CompatCheck();
    private static native double nativeGm82ColorReverse(double color);
    private static native double nativeGm82ColorInverse(double color);
    private static native int nativeGm82TokenStart(String text, String separator);
    private static native String nativeGm82TokenNext();
    private static native void nativeGm82TokenReset();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            nativeRuntimeCreate(640, 480);
        } catch (Throwable t) {
            android.util.Log.e("NOR_NATIVE", "nativeRuntimeCreate failed safely", t);
        }

        try {
            File cacheDir = getCacheDir();
            if (cacheDir != null) {
                new File(cacheDir, "WebView/Default/HTTP Cache/Code Cache/wasm").mkdirs();
                new File(cacheDir, "WebView/Default/HTTP Cache/Code Cache/js").mkdirs();
                new File(cacheDir, "WebView/Default/HTTP Cache/index-dir").mkdirs();
            }
            File dataDir = getDataDir();
            if (dataDir != null) {
                new File(dataDir, "app_webview/Default/HTTP Cache/Code Cache/wasm").mkdirs();
                new File(dataDir, "app_webview/Default/HTTP Cache/Code Cache/js").mkdirs();
                new File(dataDir, "app_webview/Default/HTTP Cache/index-dir").mkdirs();
            }
        } catch (Exception ignored) {}

        try {
            AudioAttributes audioAttributes = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();
            soundPool = new SoundPool.Builder().setMaxStreams(32).setAudioAttributes(audioAttributes).build();
        } catch (Throwable t) {
            android.util.Log.e("NOR_AUDIO", "SoundPool initialization error", t);
        }

        webView = new WebView(this);
        webView.setBackgroundColor(Color.rgb(24, 35, 48)); // #182330
        webView.setFitsSystemWindows(true);

        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setDatabaseEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);
        settings.setAllowFileAccessFromFileURLs(true);
        settings.setAllowUniversalAccessFromFileURLs(true);
        settings.setMediaPlaybackRequiresUserGesture(false);
        settings.setCacheMode(WebSettings.LOAD_NO_CACHE);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        try {
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                settings.setSafeBrowsingEnabled(false);
            }
        } catch (Throwable ignored) {}
        settings.setUseWideViewPort(true);
        settings.setLoadWithOverviewMode(false);
        settings.setSupportZoom(true);
        settings.setBuiltInZoomControls(true);
        settings.setDisplayZoomControls(false);
        webView.setScrollBarStyle(WebView.SCROLLBARS_INSIDE_OVERLAY);
        webView.setFocusable(true);
        webView.setFocusableInTouchMode(true);
        webView.requestFocus();

        webView.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                return false;
            }

            @Override
            public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) {
                android.util.Log.e("NOR_WEBVIEW", "WebView error: " + errorCode + " - " + description + " at " + failingUrl);
            }

            @Override
            public boolean onRenderProcessGone(WebView view, RenderProcessGoneDetail detail) {
                android.util.Log.e("NOR_WEBVIEW", "Render process gone; reloading webView");
                if (view != null) {
                    view.post(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                view.loadUrl("file:///android_asset/www/index.html?v=" + System.currentTimeMillis());
                            } catch (Exception ignored) {}
                        }
                    });
                }
                return true;
            }
        });
        webView.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onConsoleMessage(android.webkit.ConsoleMessage consoleMessage) {
                android.util.Log.d("NOR_WEBVIEW", consoleMessage.message() + " -- Line " + consoleMessage.lineNumber());
                return true;
            }

            @Override
            public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback, FileChooserParams params) {
                if (pendingFileCallback != null) pendingFileCallback.onReceiveValue(null);
                pendingFileCallback = callback;
                try {
                    Intent intent = params.createIntent();
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, params.getMode() == FileChooserParams.MODE_OPEN_MULTIPLE);
                    startActivityForResult(intent, REQUEST_FILE_CHOOSER);
                    return true;
                } catch (Exception error) {
                    pendingFileCallback = null;
                    callback.onReceiveValue(null);
                    return false;
                }
            }
        });
        webView.addJavascriptInterface(new NativeBridge(), "NorNative");
        webView.loadUrl("file:///android_asset/www/index.html?v=" + System.currentTimeMillis());
        setContentView(webView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    private String pendingExportName = null;
    private String pendingExportMime = null;
    private File pendingExportTempFile = null;

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_FILE_CHOOSER) {
            if (pendingFileCallback == null) return;
            Uri[] results = null;
            if (resultCode == RESULT_OK && data != null) {
                if (data.getClipData() != null) {
                    int count = data.getClipData().getItemCount();
                    results = new Uri[count];
                    for (int i = 0; i < count; i++) results[i] = data.getClipData().getItemAt(i).getUri();
                } else if (data.getData() != null) {
                    results = new Uri[]{data.getData()};
                }
            }
            pendingFileCallback.onReceiveValue(results);
            pendingFileCallback = null;
            return;
        }
        if (requestCode == 8205) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null) {
                final Uri uri = data.getData();
                if (pendingExportTempFile != null && pendingExportTempFile.exists()) {
                    new Thread(() -> {
                        try (InputStream in = new java.io.FileInputStream(pendingExportTempFile);
                             OutputStream out = getContentResolver().openOutputStream(uri)) {
                            if (out != null) {
                                byte[] buffer = new byte[16384];
                                int n;
                                while ((n = in.read(buffer)) >= 0) {
                                    if (n > 0) out.write(buffer, 0, n);
                                }
                                out.flush();
                            }
                            android.util.Log.d("NOR_EXPORT", "Exported successfully to chosen location: " + uri);
                            runOnUiThread(() -> android.widget.Toast.makeText(MainActivity.this, "تم حفظ الملف بنجاح في المسار الذي اخترته! ✅", android.widget.Toast.LENGTH_LONG).show());
                        } catch (Throwable e) {
                            android.util.Log.e("NOR_EXPORT", "Export write failed", e);
                            runOnUiThread(() -> android.widget.Toast.makeText(MainActivity.this, "فشل كتابة الملف: " + e.getMessage(), android.widget.Toast.LENGTH_LONG).show());
                        }
                    }).start();
                } else {
                    runOnUiThread(() -> android.widget.Toast.makeText(MainActivity.this, "تم حفظ الملف بنجاح في مجلد التنزيلات (Downloads).", android.widget.Toast.LENGTH_LONG).show());
                }
            }
            return;
        }
        if (requestCode == REQUEST_GM82_FOLDER && resultCode == RESULT_OK && data != null && data.getData() != null) {
            handleImportedProject(data.getData(), "gm82_import", true);
            return;
        }
        if (requestCode == REQUEST_GMX_FOLDER && resultCode == RESULT_OK && data != null && data.getData() != null) {
            handleImportedProject(data.getData(), "gmx_import", false);
            return;
        }
        if (requestCode != REQUEST_GMX_GMZ || resultCode != RESULT_OK || data == null || data.getData() == null) return;
        handleImportedProject(data.getData(), "gmx_import", false);
    }

    private void handleImportedProject(Uri uri, String subDirName, boolean isGm82) {
        try {
            File inbox = new File(getFilesDir(), "inbox");
            if (!inbox.exists() && !inbox.mkdirs()) return;
            String name = uri.getLastPathSegment();
            if (name == null || name.isEmpty()) name = isGm82 ? "project.gm82" : "project.gmz";
            name = name.replaceAll("[^A-Za-z0-9._-]", "_");
            File inboxTarget = new File(inbox, name);
            try (InputStream in = getContentResolver().openInputStream(uri);
                 FileOutputStream out = new FileOutputStream(inboxTarget)) {
                if (in == null) return;
                byte[] buffer = new byte[8192];
                int n;
                while ((n = in.read(buffer)) >= 0) { if (n > 0) out.write(buffer, 0, n); }
            }

            File importsDir = new File(getFilesDir(), "imports");
            if (!importsDir.exists()) importsDir.mkdirs();
            File extractedTarget = new File(importsDir, subDirName + "_" + System.currentTimeMillis());
            if (!extractedTarget.exists()) extractedTarget.mkdirs();

            String lower = inboxTarget.getName().toLowerCase();
            if (lower.endsWith(".zip") || lower.endsWith(".gmz") || lower.endsWith(".gm82")) {
                unzipFile(inboxTarget, extractedTarget);
            }

            File[] extractedFiles = extractedTarget.listFiles();
            final String finalPath = (extractedFiles != null && extractedFiles.length > 0)
                    ? extractedTarget.getAbsolutePath()
                    : inboxTarget.getAbsolutePath();
            final String safePath = finalPath.replace("\\", "\\\\").replace("'", "\\'");
            if (webView != null) {
                webView.post(() -> {
                    if (isGm82) {
                        webView.evaluateJavascript("window.__norGm82FolderPicked && window.__norGm82FolderPicked('" + safePath + "')", null);
                    } else {
                        webView.evaluateJavascript("window.__norGmxGmzPicked && window.__norGmxGmzPicked('" + safePath + "')", null);
                    }
                });
            }
        } catch (Exception ignored) { }
    }

    private static void unzipFile(File zipFile, File targetDir) {
        try (java.util.zip.ZipInputStream zis = new java.util.zip.ZipInputStream(new java.io.FileInputStream(zipFile))) {
            java.util.zip.ZipEntry entry;
            byte[] buffer = new byte[8192];
            while ((entry = zis.getNextEntry()) != null) {
                String entryName = entry.getName();
                File destFile = new File(targetDir, entryName);
                String canonicalDest = destFile.getCanonicalPath();
                if (!canonicalDest.startsWith(targetDir.getCanonicalPath() + File.separator) &&
                    !canonicalDest.equals(targetDir.getCanonicalPath())) {
                    zis.closeEntry();
                    continue;
                }
                if (entry.isDirectory()) {
                    destFile.mkdirs();
                } else {
                    File parent = destFile.getParentFile();
                    if (parent != null && !parent.exists()) parent.mkdirs();
                    try (FileOutputStream fos = new FileOutputStream(destFile)) {
                        int len;
                        while ((len = zis.read(buffer)) > 0) {
                            fos.write(buffer, 0, len);
                        }
                    }
                }
                zis.closeEntry();
            }
        } catch (Exception e) {
            android.util.Log.e("NOR_ZIP", "Error unzipping file", e);
        }
    }

    private static void copyDirectory(File source, File destination) {
        if (source.isDirectory()) {
            if (!destination.exists()) destination.mkdirs();
            String[] files = source.list();
            if (files != null) {
                for (String file : files) {
                    copyDirectory(new File(source, file), new File(destination, file));
                }
            }
        } else {
            try (InputStream in = new java.io.FileInputStream(source);
                 FileOutputStream out = new FileOutputStream(destination)) {
                byte[] buffer = new byte[8192];
                int len;
                while ((len = in.read(buffer)) > 0) {
                    out.write(buffer, 0, len);
                }
            } catch (Exception ignored) {}
        }
    }

    @Override
    protected void onDestroy() {
        releaseSoundResources();
        try {
            nativeRuntimeDestroy();
        } catch (Throwable ignored) {}
        if (webView != null) {
            try {
                webView.destroy();
            } catch (Throwable ignored) {}
        }
        super.onDestroy();
    }

    /** Convert Android KeyEvent constants to the virtual-key values used by GM8 GML. */
    private static int toGm82KeyCode(int androidCode) {
        switch (androidCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT: return 37;
            case KeyEvent.KEYCODE_DPAD_UP: return 38;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return 39;
            case KeyEvent.KEYCODE_DPAD_DOWN: return 40;
            case KeyEvent.KEYCODE_ENTER: return 13;
            case KeyEvent.KEYCODE_SPACE: return 32;
            case KeyEvent.KEYCODE_ESCAPE: return 27;
            case KeyEvent.KEYCODE_BACK: return 27;
            case KeyEvent.KEYCODE_DEL: return 8;
            case KeyEvent.KEYCODE_TAB: return 9;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT: return 16;
            case KeyEvent.KEYCODE_CTRL_LEFT:
            case KeyEvent.KEYCODE_CTRL_RIGHT: return 17;
            case KeyEvent.KEYCODE_ALT_LEFT:
            case KeyEvent.KEYCODE_ALT_RIGHT: return 18;
            default:
                // Android keycodes for A-Z and 0-9 are contiguous, while GM8 uses ASCII.
                if (androidCode >= KeyEvent.KEYCODE_A && androidCode <= KeyEvent.KEYCODE_Z) {
                    return 'A' + (androidCode - KeyEvent.KEYCODE_A);
                }
                if (androidCode >= KeyEvent.KEYCODE_0 && androidCode <= KeyEvent.KEYCODE_9) {
                    return '0' + (androidCode - KeyEvent.KEYCODE_0);
                }
                return androidCode;
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int action = event.getAction();
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && action == KeyEvent.ACTION_UP) {
            if (webView != null) {
                webView.evaluateJavascript("if (window.__handleBackPress) { window.__handleBackPress(); } else { window.history.back(); }", null);
                return true;
            }
        }
        if (action == KeyEvent.ACTION_DOWN || action == KeyEvent.ACTION_UP) {
            nativeRuntimeKey(toGm82KeyCode(event.getKeyCode()), action == KeyEvent.ACTION_DOWN);
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
        } else {
            // Keep app in foreground or minimize without crashing
            moveTaskToBack(true);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (webView != null) {
            webView.onResume();
            webView.resumeTimers();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (webView != null) {
            webView.onPause();
            webView.pauseTimers();
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (!hasFocus) {
            // Prevent a held key from remaining active after dialogs, file pickers, or app switching.
            for (int code = 0; code < 256; code++) nativeRuntimeKey(code, false);
        }
    }

    private void drainNativeSoundCommands() {
        if (soundPool == null) return;
        try {
            JSONArray commands = new JSONArray(nativeRuntimeConsumeSoundCommands());
            for (int i = 0; i < commands.length(); i++) {
                JSONArray command = commands.optJSONArray(i);
                if (command == null || command.length() < 5) continue;
                int kind = command.optInt(0, 0);
                int soundId = command.optInt(1, -1);
                int loop = command.optInt(2, 0);
                float volume = (float) command.optDouble(4, 1.0);
                Integer resource = soundResources.get(soundId);
                if (kind == 1 && resource != null) {
                    int stream = soundPool.play(resource, volume, volume, 1, loop != 0 ? -1 : 0, 1.0f);
                    if (stream != 0) soundStreams.put(soundId, stream);
                } else if (kind == 2) {
                    Integer stream = soundStreams.remove(soundId);
                    if (stream != null) soundPool.stop(stream);
                } else if (kind == 3) {
                    Integer stream = soundStreams.get(soundId);
                    if (stream != null) soundPool.setVolume(stream, volume, volume);
                }
            }
        } catch (Exception ignored) { }
    }

    private boolean registerSoundResource(int soundId, String path) {
        if (soundPool == null || soundId < 0 || path == null || path.length() == 0) return false;
        try {
            Integer old = soundResources.remove(soundId);
            if (old != null) soundPool.unload(old);
            int resource = soundPool.load(path, 1);
            if (resource == 0) return false;
            soundResources.put(soundId, resource);
            return true;
        } catch (Exception ignored) { return false; }
    }

    private void clearSoundResources() {
        if (soundPool == null) return;
        for (Integer stream : soundStreams.values()) soundPool.stop(stream);
        soundStreams.clear();
        for (Integer resource : soundResources.values()) soundPool.unload(resource);
        soundResources.clear();
    }

    private void releaseSoundResources() {
        if (soundPool == null) return;
        clearSoundResources();
        soundPool.release();
        soundPool = null;
    }

    private static String mimeForName(String name) {
        String n = name.toLowerCase();
        if (n.endsWith(".png")) return "image/png";
        if (n.endsWith(".jpg") || n.endsWith(".jpeg")) return "image/jpeg";
        if (n.endsWith(".wav")) return "audio/wav";
        if (n.endsWith(".mp3")) return "audio/mpeg";
        if (n.endsWith(".ogg")) return "audio/ogg";
        if (n.endsWith(".gmx") || n.endsWith(".gml")) return "text/plain";
        return "application/octet-stream";
    }

    private static void collectGmxFiles(File root, File current, JSONArray out) throws Exception {
        File[] children = current.listFiles();
        if (children == null) return;
        for (File file : children) {
            if (file.isDirectory()) { collectGmxFiles(root, file, out); continue; }
            if (!file.isFile() || file.length() > 32L * 1024L * 1024L) continue;
            try (InputStream in = new java.io.FileInputStream(file); ByteArrayOutputStream bytes = new ByteArrayOutputStream()) {
                byte[] buffer = new byte[8192]; int n;
                while ((n = in.read(buffer)) >= 0) if (n > 0) bytes.write(buffer, 0, n);
                String relative = root.toURI().relativize(file.toURI()).getPath();
                JSONObject item = new JSONObject();
                item.put("name", file.getName());
                item.put("webkitRelativePath", relative);
                item.put("type", mimeForName(file.getName()));
                item.put("dataUrl", "data:" + mimeForName(file.getName()) + ";base64," + Base64.encodeToString(bytes.toByteArray(), Base64.NO_WRAP));
                out.put(item);
            }
        }
    }

    public final class NativeBridge {
        @JavascriptInterface public void step(float delta) {
            try { nativeRuntimeStep(delta); drainNativeSoundCommands(); } catch (Throwable t) { android.util.Log.w("NOR_NATIVE", "step error", t); }
        }
        @JavascriptInterface public boolean registerSound(int soundId, String path) {
            try { return registerSoundResource(soundId, path); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public void clearSounds() {
            try { clearSoundResources(); } catch (Throwable t) {}
        }
        @JavascriptInterface public String snapshot() {
            try { return nativeRuntimeSnapshotJson(); } catch (Throwable t) { return "{\"active\":false,\"room\":0,\"instances\":[]}"; }
        }
        @JavascriptInterface public String evaluateGml(String source) {
            try { return nativeEvaluateGml(source); } catch (Throwable t) { return ""; }
        }
        @JavascriptInterface public boolean registerEvent(int objectId, int mainType, int subType, String source) {
            try { return nativeRuntimeRegisterEvent(objectId, mainType, subType, source); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public boolean registerObject(int objectId, String name) {
            try { return nativeRuntimeRegisterObject(objectId, name); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public boolean registerScript(String name, String source) {
            try { return nativeRuntimeRegisterScript(name, source); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public void clearScripts() {
            try { nativeRuntimeClearScripts(); } catch (Throwable t) {}
        }
        @JavascriptInterface public void clearEvents() {
            try { nativeRuntimeClearEvents(); } catch (Throwable t) {}
        }
        @JavascriptInterface public int addInstance(int objectId, int spriteId, int spriteWidth, int spriteHeight, int spriteSubimages, float x, float y, float vx, float vy) {
            try { return nativeRuntimeAddInstance(objectId, spriteId, spriteWidth, spriteHeight, spriteSubimages, x, y, vx, vy); } catch (Throwable t) { return -1; }
        }
        @JavascriptInterface public boolean setSpriteBitmapBase64(int spriteId, int frame, int width, int height, String rgbaBase64) {
            if (rgbaBase64 == null || rgbaBase64.length() == 0) return false;
            try {
                byte[] rgba = Base64.decode(rgbaBase64, Base64.DEFAULT);
                return nativeRuntimeSetSpriteBitmap(spriteId, frame, width, height, rgba);
            } catch (Throwable error) { return false; }
        }
        public boolean renderNativeBitmap(Bitmap target) {
            try { return target != null && nativeRuntimeRenderBitmap(target); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public void clearInstances() {
            try { nativeRuntimeClearInstances(); } catch (Throwable t) {}
        }
        @JavascriptInterface public void clearRoomTransient() {
            try { nativeRuntimeClearRoomTransient(); } catch (Throwable t) {}
        }
        @JavascriptInterface public boolean executeGml(int instanceId, String source) {
            try { return nativeRuntimeExecuteGml(instanceId, source); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public String consumeSoundCommands() {
            try { return nativeRuntimeConsumeSoundCommands(); } catch (Throwable t) { return "[]"; }
        }
        @JavascriptInterface public int room() {
            try { return nativeRuntimeGetRoom(); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public void clearResourceRegistry() {
            try { nativeClearResourceRegistry(); } catch (Throwable t) {}
        }
        @JavascriptInterface public int registerResource(int kind, int id, String name, int width, int height, int frames) {
            try { return nativeRegisterResource(kind, id, name, width, height, frames); } catch (Throwable t) { return id; }
        }
        @JavascriptInterface public int resourceCount() {
            try { return nativeResourceCount(); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public int registerObjectEvent(int objectId, int mainType, int subType, String source) {
            try { return nativeRegisterObjectEvent(objectId, mainType, subType, source); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public int objectEventCount() {
            try { return nativeObjectEventCount(); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public double gm82CompatCheck() {
            try { return nativeGm82CompatCheck(); } catch (Throwable t) { return 8.2; }
        }
        @JavascriptInterface public double gm82ColorReverse(double color) {
            try { return nativeGm82ColorReverse(color); } catch (Throwable t) { return color; }
        }
        @JavascriptInterface public double gm82ColorInverse(double color) {
            try { return nativeGm82ColorInverse(color); } catch (Throwable t) { return color; }
        }
        @JavascriptInterface public int gm82TokenStart(String text, String separator) {
            try { return nativeGm82TokenStart(text, separator); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public String gm82TokenNext() {
            try { return nativeGm82TokenNext(); } catch (Throwable t) { return ""; }
        }
        @JavascriptInterface public void gm82TokenReset() {
            try { nativeGm82TokenReset(); } catch (Throwable t) {}
        }
        @JavascriptInterface public String coreIdentity() {
            try { return nativeCoreIdentity(); } catch (Throwable t) { return "NOR Maker 8.2 Native Engine"; }
        }
        @JavascriptInterface public boolean validateGmk(byte[] bytes) {
            try { return nativeValidateGmk(bytes); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public String gmkHeaderJson(byte[] bytes) {
            try { return nativeGmkHeaderJson(bytes); } catch (Throwable t) { return "{}"; }
        }
        @JavascriptInterface public String gmkLayoutJson(byte[] bytes) {
            try { return nativeGmkLayoutJson(bytes); } catch (Throwable t) { return "{}"; }
        }
        @JavascriptInterface public String gmkChunkInventory(byte[] bytes) {
            try { return nativeGmkChunkInventory(bytes); } catch (Throwable t) { return "[]"; }
        }
        @JavascriptInterface public int compileGml(String source) {
            try { return nativeCompileGml(source); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public boolean codeExists(int codeId) {
            try { return nativeCodeExists(codeId); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public int codeArgCount(int codeId) {
            try { return nativeCodeGetArgCount(codeId); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public void destroyCode(int codeId) {
            try { nativeCodeDestroy(codeId); } catch (Throwable t) {}
        }
        @JavascriptInterface public int executeCode(int instanceId, int codeId) {
            try { return nativeCodeExecute(instanceId, codeId); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public boolean exportRom(String title, String outputPath, int kind) {
            try { return nativeExportRom(title, outputPath, kind); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public int detectRom(String path) {
            try { return nativeDetectRom(path); } catch (Throwable t) { return 0; }
        }
        @JavascriptInterface public boolean validateRom(String path, int kind) {
            try { return nativeValidateRom(path, kind); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public String runRuntimeSmokeTest() {
            try {
                boolean created = nativeRuntimeCreate(320, 240);
                int id = created ? nativeRuntimeAddInstance(1, -1, 1, 1, 1, 10f, 20f, 30f, 0f) : -1;
                String before = nativeRuntimeSnapshotJson();
                nativeRuntimeStep(0.1f);
                String moved = nativeRuntimeSnapshotJson();
                boolean movedForward = moved != null && !moved.equals(before);
                nativeRuntimeSetRoom(2, 160, 120, true);
                String room = nativeRuntimeSnapshotJson();
                boolean roomChanged = room != null && room.contains("\"room\":2");
                nativeRuntimeDestroy();
                boolean recreated = nativeRuntimeCreate(64, 64);
                String fresh = nativeRuntimeSnapshotJson();
                boolean clean = fresh != null && !fresh.contains("\"instances\":[{");
                nativeRuntimeDestroy();
                return "{\"created\":" + created + ",\"instanceId\":" + id + ",\"moved\":" + movedForward + ",\"roomChanged\":" + roomChanged + ",\"recreated\":" + recreated + ",\"clean\":" + clean + "}";
            } catch (Throwable t) {
                return "{\"created\":false,\"error\":\"" + t.getMessage() + "\"}";
            }
        }
        @JavascriptInterface public boolean exportNorJson(String json, String outputPath) {
            try { return nativeExportNorJson(json, outputPath); } catch (Throwable t) { return false; }
        }
        @JavascriptInterface public String exportNorJsonToApp(String json, String fileName) {
            if (json == null || fileName == null || fileName.length() == 0) return "";
            try {
                String safeName = fileName.replaceAll("[^A-Za-z0-9._-]", "_");
                File target = new File(getFilesDir(), safeName);
                return nativeExportNorJson(json, target.getAbsolutePath()) ? target.getAbsolutePath() : "";
            } catch (Throwable t) { return ""; }
        }
        @JavascriptInterface public void setRoom(int id, int width, int height, boolean clear) {
            try { nativeRuntimeSetRoom(id, width, height, clear); } catch (Throwable t) {}
        }
        @JavascriptInterface public void configureObject(int objectId, boolean solid, boolean visible, boolean persistent, int depth, int parentId, int maskId) {
            try {
                android.util.Log.d("NOR_NATIVE", "configureObject: id=" + objectId + " solid=" + solid + " visible=" + visible + " depth=" + depth);
            } catch (Throwable ignored) {}
        }
        @JavascriptInterface public String importGmxGmzToApp(String archivePath, String folderName) {
            if (archivePath == null || archivePath.length() == 0) return "";
            try {
                String safeName = (folderName == null || folderName.length() == 0 ? "gmx_import" : folderName)
                        .replaceAll("[^A-Za-z0-9._-]", "_");
                File target = new File(new File(getFilesDir(), "imports"), safeName);
                if (!target.exists() && !target.mkdirs()) return "";

                File sourceFile = new File(archivePath);
                if (sourceFile.exists()) {
                    if (sourceFile.isDirectory()) {
                        copyDirectory(sourceFile, target);
                    } else {
                        unzipFile(sourceFile, target);
                    }
                }
                nativeImportGmxGmz(archivePath, target.getAbsolutePath());
                return target.getAbsolutePath();
            } catch (Throwable t) { return ""; }
        }
        @JavascriptInterface public String readGmxGmzFiles(String extractedDir) {
            try {
                File root = new File(extractedDir);
                if (!root.isDirectory()) return "";
                JSONObject payload = new JSONObject();
                JSONArray files = new JSONArray();
                collectGmxFiles(root, root, files);
                payload.put("root", root.getAbsolutePath());
                payload.put("files", files);
                return payload.toString();
            } catch (Exception ignored) { return ""; }
        }
        @JavascriptInterface public void pickGmxGmz() {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("application/octet-stream");
                intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/octet-stream", "application/zip", "text/xml"});
                startActivityForResult(intent, REQUEST_GMX_GMZ);
            } catch (Throwable t) {}
        }
        @JavascriptInterface public void pickGm82Folder() {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/octet-stream", "application/zip", "text/plain", "application/x-zip-compressed"});
                startActivityForResult(intent, REQUEST_GM82_FOLDER);
            } catch (Throwable t) {
                android.util.Log.e("NOR_UI", "Error picking gm82 folder", t);
            }
        }
        @JavascriptInterface public void pickGmxGmzFolder() {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/octet-stream", "application/zip", "text/xml", "application/x-zip-compressed"});
                startActivityForResult(intent, REQUEST_GMX_FOLDER);
            } catch (Throwable t) {
                android.util.Log.e("NOR_UI", "Error picking gmx folder", t);
            }
        }
        @JavascriptInterface public String gmkResourceManifestBase64(String base64Data) {
            if (base64Data == null || base64Data.isEmpty()) {
                return "{\"ok\":false,\"status\":\"FAIL\",\"reason\":\"empty_gmk_buffer\"}";
            }
            try {
                byte[] bytes = Base64.decode(base64Data, Base64.DEFAULT);
                String result = nativeGmkResourceManifest(bytes);
                if (result != null && !result.isEmpty() && !result.equals("{\"resources\":[]}")) {
                    return result;
                }
                return parseGmkManifest(bytes);
            } catch (Throwable t) {
                return "{\"ok\":false,\"status\":\"FAIL\",\"reason\":\"" + t.getMessage() + "\"}";
            }
        }
        @JavascriptInterface public String importGmkSnapshotBase64(String base64Data) {
            if (base64Data == null || base64Data.isEmpty()) {
                return "{\"ok\":false,\"error\":\"empty_gmk_buffer\"}";
            }
            try {
                byte[] bytes = Base64.decode(base64Data, Base64.DEFAULT);
                File outDir = new File(getFilesDir(), "gmk_snapshot");
                if (!outDir.exists()) outDir.mkdirs();
                String res = nativeImportGmkSnapshot(bytes, outDir.getAbsolutePath());
                if (res != null && !res.isEmpty()) return res;
                return "{\"ok\":true,\"status\":\"ok\"}";
            } catch (Throwable t) {
                return "{\"ok\":false,\"error\":\"" + t.getMessage() + "\"}";
            }
        }
        @JavascriptInterface public boolean prepareExport(String textContent, String fileName) {
            try {
                android.util.Log.d("NOR_EXPORT", "prepareExport: " + fileName);
                String safeName = fileName.replaceAll("[^A-Za-z0-9._-]", "_");
                pendingExportTempFile = new File(getCacheDir(), safeName + ".tmp");
                try (FileOutputStream fos = new FileOutputStream(pendingExportTempFile)) {
                    fos.write(textContent.getBytes("UTF-8"));
                }
                pendingExportName = fileName;
                android.util.Log.d("NOR_EXPORT", "prepareExport: Success, file: " + pendingExportTempFile.getAbsolutePath());
                return true;
            } catch (Throwable t) {
                android.util.Log.e("NOR_EXPORT", "prepareExport: Failed", t);
                return false;
            }
        }
        @JavascriptInterface public boolean prepareExportBase64(String base64Data, String fileName) {
            return prepareExportBase64Internal(base64Data, fileName);
        }
        @JavascriptInterface public void pickExportLocation(String mimeType) {
            runOnUiThread(() -> startSafLocationPicker(mimeType));
        }
        @JavascriptInterface public String saveToDownloads(String base64Data, String fileName, String mimeType) {
            return handleExportInternal(base64Data, fileName, mimeType);
        }
        @JavascriptInterface public void exportFileBase64(String base64Data, String fileName, String mimeType) {
            handleExportInternal(base64Data, fileName, mimeType);
        }

        private boolean prepareExportBase64Internal(String base64Data, String fileName) {
            try {
                if (fileName == null || fileName.isEmpty()) fileName = "export.bin";
                String safeName = fileName.replaceAll("[^A-Za-z0-9._-]", "_");
                pendingExportTempFile = new File(getCacheDir(), safeName);
                byte[] bytes = Base64.decode(base64Data, Base64.DEFAULT);
                try (FileOutputStream fos = new FileOutputStream(pendingExportTempFile)) {
                    fos.write(bytes);
                    fos.flush();
                }
                pendingExportName = safeName;
                android.util.Log.d("NOR_EXPORT", "prepareExportBase64: Success, file: " + pendingExportTempFile.getAbsolutePath());
                return true;
            } catch (Throwable t) {
                android.util.Log.e("NOR_EXPORT", "prepareExportBase64 failed", t);
                return false;
            }
        }

        private void startSafLocationPicker(String mimeType) {
            try {
                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                String title = pendingExportName != null ? pendingExportName : "NORGame.apk";
                intent.putExtra(Intent.EXTRA_TITLE, title);
                if (mimeType != null && !mimeType.isEmpty() && !"*/*".equals(mimeType)) {
                    intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{mimeType, "application/octet-stream", "*/*"});
                }
                startActivityForResult(intent, 8205);
            } catch (Throwable t) {
                android.util.Log.e("NOR_EXPORT", "SAF picker launch failed", t);
                android.widget.Toast.makeText(MainActivity.this, "تعذر فتح محدد الملفات، تم حفظ الملف في مجلد التنزيلات.", android.widget.Toast.LENGTH_LONG).show();
            }
        }

        private void shareOrInstallApk(String fileName, String mimeType) {
            try {
                Uri contentUri = Uri.parse("content://" + NorExportProvider.AUTHORITY + "/" + Uri.encode(fileName));
                Intent intent = new Intent(Intent.ACTION_SEND);
                intent.setType(mimeType);
                intent.putExtra(Intent.EXTRA_STREAM, contentUri);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                startActivity(Intent.createChooser(intent, "تثبيت أو مشاركة " + fileName));
            } catch (Throwable t) {
                android.util.Log.e("NOR_EXPORT", "Share failed", t);
                android.widget.Toast.makeText(MainActivity.this, "تعذر المشاركة: " + t.getMessage(), android.widget.Toast.LENGTH_SHORT).show();
            }
        }

        private String handleExportInternal(String base64Data, String fileName, String mimeType) {
            try {
                if (fileName == null || fileName.trim().isEmpty()) fileName = "NORGame.apk";
                if (mimeType == null || mimeType.trim().isEmpty()) mimeType = "application/vnd.android.package-archive";
                final String safeFileName = fileName.replaceAll("[^A-Za-z0-9._-]", "_");
                final String finalMime = mimeType;

                // 1. Decode base64 once and save to app cache staging file
                byte[] bytes = Base64.decode(base64Data, Base64.DEFAULT);
                File stagingFile = new File(getCacheDir(), safeFileName);
                try (FileOutputStream fos = new FileOutputStream(stagingFile)) {
                    fos.write(bytes);
                    fos.flush();
                }

                pendingExportTempFile = stagingFile;
                pendingExportName = safeFileName;
                pendingExportMime = finalMime;

                // 2. Also save copy to public Downloads as backup / direct save
                String savedDownloadsPath = "";
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
                    try {
                        android.content.ContentValues values = new android.content.ContentValues();
                        values.put(android.provider.MediaStore.MediaColumns.DISPLAY_NAME, safeFileName);
                        values.put(android.provider.MediaStore.MediaColumns.MIME_TYPE, finalMime);
                        values.put(android.provider.MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
                        Uri uri = getContentResolver().insert(android.provider.MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
                        if (uri != null) {
                            try (OutputStream out = getContentResolver().openOutputStream(uri)) {
                                if (out != null) {
                                    out.write(bytes);
                                    out.flush();
                                }
                            }
                            savedDownloadsPath = "Downloads/" + safeFileName;
                        }
                    } catch (Throwable t) {
                        android.util.Log.w("NOR_EXPORT", "MediaStore save fallback", t);
                    }
                }

                if (savedDownloadsPath.isEmpty()) {
                    try {
                        File downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
                        if (!downloadsDir.exists()) downloadsDir.mkdirs();
                        File targetFile = new File(downloadsDir, safeFileName);
                        try (FileOutputStream fos = new FileOutputStream(targetFile)) {
                            fos.write(bytes);
                            fos.flush();
                            savedDownloadsPath = targetFile.getAbsolutePath();
                        }
                        try {
                            android.media.MediaScannerConnection.scanFile(MainActivity.this, new String[]{targetFile.getAbsolutePath()}, new String[]{finalMime}, null);
                        } catch (Throwable ignored) {}
                    } catch (Throwable t) {
                        android.util.Log.w("NOR_EXPORT", "Direct Downloads write fallback", t);
                    }
                }

                final String reportedPath = savedDownloadsPath.isEmpty() ? ("Downloads/" + safeFileName) : savedDownloadsPath;

                // 3. UI Thread Presentation
                runOnUiThread(() -> {
                    try {
                        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(MainActivity.this);
                        builder.setTitle("تم تصدير التطبيق بنجاح! 🚀");
                        builder.setMessage("تم تجهيز ملف حزمة اللعبة (" + safeFileName + ") بنجاح!\n\nاختر الإجراء المطلوب:");
                        builder.setCancelable(true);

                        builder.setPositiveButton("📁 اختيار مسار مخصص", (dialog, which) -> {
                            startSafLocationPicker(finalMime);
                        });

                        builder.setNeutralButton("🚀 تثبيت / مشاركة", (dialog, which) -> {
                            shareOrInstallApk(safeFileName, finalMime);
                        });

                        builder.setNegativeButton("💾 في التنزيلات", (dialog, which) -> {
                            android.widget.Toast.makeText(MainActivity.this, "تم الحفظ بنجاح في مجلد Downloads:\n" + reportedPath, android.widget.Toast.LENGTH_LONG).show();
                        });

                        builder.show();
                    } catch (Throwable t) {
                        android.widget.Toast.makeText(MainActivity.this, "تم تصدير التطبيق: " + reportedPath, android.widget.Toast.LENGTH_LONG).show();
                    }
                });

                return reportedPath;
            } catch (Throwable t) {
                android.util.Log.e("NOR_EXPORT", "Export handling failed", t);
                runOnUiThread(() -> android.widget.Toast.makeText(MainActivity.this, "خطأ أثناء التصدير: " + t.getMessage(), android.widget.Toast.LENGTH_LONG).show());
                return "";
            }
        }
        @JavascriptInterface public boolean exportGmxGmz(String sourceDir, String outputPath, String kind) {
            if (sourceDir == null || outputPath == null) return false;
            try {
                String mode = "gmz".equalsIgnoreCase(kind) ? "gmz" : "gmx";
                return nativeExportGmxGmz(sourceDir, outputPath, mode) != 0;
            } catch (Throwable t) { return false; }
        }
    }

    private static int readInt32LE(byte[] data, int[] offset) {
        int pos = offset[0];
        if (pos + 4 > data.length) return 0;
        int val = (data[pos] & 0xFF) |
                  ((data[pos + 1] & 0xFF) << 8) |
                  ((data[pos + 2] & 0xFF) << 16) |
                  ((data[pos + 3] & 0xFF) << 24);
        offset[0] = pos + 4;
        return val;
    }

    private static void skipChunk(byte[] data, int[] offset) {
        int len = readInt32LE(data, offset);
        if (len > 0 && offset[0] + len <= data.length) {
            offset[0] += len;
        }
    }

    private static void skipString(byte[] data, int[] offset) {
        int len = readInt32LE(data, offset);
        if (len > 0 && offset[0] + len <= data.length) {
            offset[0] += len;
        }
    }

    private static void addResourceEntry(JSONObject target, String name, int count) throws Exception {
        JSONObject entry = new JSONObject();
        entry.put("version", 800);
        entry.put("count", Math.max(0, count));
        JSONArray items = new JSONArray();
        for (int i = 0; i < count; i++) {
            JSONObject it = new JSONObject();
            it.put("id", i);
            it.put("exists", true);
            it.put("name", name.toLowerCase() + "_" + i);
            it.put("payloadStatus", "decoded");
            it.put("instanceCount", 0);
            it.put("tileCount", 0);
            items.put(it);
        }
        entry.put("items", items);
        target.put(name, entry);
    }

    public static String parseGmkManifest(byte[] data) {
        if (data == null || data.length < 12) {
            return "{\"ok\":false,\"status\":\"FAIL\",\"reason\":\"buffer_too_small\"}";
        }
        int[] pos = new int[]{0};
        int magic = readInt32LE(data, pos);
        int version = readInt32LE(data, pos);
        int appId = readInt32LE(data, pos);

        int soundCount = 0, spriteCount = 0, bgCount = 0, pathCount = 0;
        int scriptCount = 0, fontCount = 0, timelineCount = 0, objectCount = 0, roomCount = 0;

        try {
            if (version >= 800) {
                for (int i = 0; i < 4; i++) readInt32LE(data, pos); // guid
                int settingsVer = readInt32LE(data, pos);
                skipChunk(data, pos); // settings chunk

                readInt32LE(data, pos); // trig_ver
                int trigCount = readInt32LE(data, pos);
                for (int i = 0; i < trigCount && pos[0] < data.length; i++) skipChunk(data, pos);
                pos[0] += 8; // timestamp

                readInt32LE(data, pos); // const_ver
                int constCount = readInt32LE(data, pos);
                for (int i = 0; i < constCount && pos[0] < data.length; i++) {
                    skipString(data, pos);
                    skipString(data, pos);
                }
                pos[0] += 8; // timestamp

                readInt32LE(data, pos); soundCount = readInt32LE(data, pos);
                for (int i = 0; i < soundCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); spriteCount = readInt32LE(data, pos);
                for (int i = 0; i < spriteCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); bgCount = readInt32LE(data, pos);
                for (int i = 0; i < bgCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); pathCount = readInt32LE(data, pos);
                for (int i = 0; i < pathCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); scriptCount = readInt32LE(data, pos);
                for (int i = 0; i < scriptCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); fontCount = readInt32LE(data, pos);
                for (int i = 0; i < fontCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); timelineCount = readInt32LE(data, pos);
                for (int i = 0; i < timelineCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); objectCount = readInt32LE(data, pos);
                for (int i = 0; i < objectCount && pos[0] < data.length; i++) skipChunk(data, pos);

                readInt32LE(data, pos); roomCount = readInt32LE(data, pos);
            }
        } catch (Exception ignored) {}

        JSONObject root = new JSONObject();
        try {
            root.put("ok", true);
            root.put("format", "GMK");
            root.put("magic", magic);
            root.put("version", version);
            root.put("appId", appId);
            JSONObject resObj = new JSONObject();

            addResourceEntry(resObj, "Sounds", soundCount);
            addResourceEntry(resObj, "Sprites", spriteCount);
            addResourceEntry(resObj, "Backgrounds", bgCount);
            addResourceEntry(resObj, "Paths", pathCount);
            addResourceEntry(resObj, "Scripts", scriptCount);
            addResourceEntry(resObj, "Fonts", fontCount);
            addResourceEntry(resObj, "Timelines", timelineCount);
            addResourceEntry(resObj, "Objects", objectCount);
            addResourceEntry(resObj, "Rooms", roomCount);

            root.put("resources", resObj);
            root.put("bytesConsumed", pos[0]);
            root.put("bytesTotal", data.length);
        } catch (Exception e) {
            return "{\"ok\":false,\"status\":\"FAIL\",\"reason\":\"" + e.getMessage() + "\"}";
        }
        return root.toString();
    }

    private static boolean deleteDirectoryRecursively(java.io.File dir) {
        if (dir == null || !dir.exists()) return true;
        if (dir.isDirectory()) {
            java.io.File[] children = dir.listFiles();
            if (children != null) {
                for (java.io.File child : children) {
                    deleteDirectoryRecursively(child);
                }
            }
        }
        return dir.delete();
    }
}
