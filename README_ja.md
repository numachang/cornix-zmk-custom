# Cornix 用 ZMK ファームウェア（日本向けカスタムビルド）

**言語:** [English](./README.md) | 日本語 | [中文](./README_zh.md)

このリポジトリ（`numachang/cornix-zmk-custom`）は、
[`hitsmaxft/zmk-keyboard-cornix`](https://github.com/hitsmaxft/zmk-keyboard-cornix) の
**日本向けダウンストリームフォーク**です。Cornix 分割キーボード用の ZMK ファームウェア設定と、
ボード/シールドモジュールを含みます。

日本では Cornix がドングルなしの**プレーンな Bluetooth 分割キーボード**として販売・使用されているため、
アップストリームから意図的に次の点だけ変更しています。

- **ドングル向けビルドターゲットを無効化**しています。左半分をセントラルとする標準的な分割構成で動作します。
  ドングル用シールドはディスク上に残っていますが、有効なビルドマトリクスには含まれません。
- **`cornix_left` で ZMK Studio を有効化**しています。USB 経由で
  [ZMK Studio](https://zmk.dev/docs/features/studio) を使い、キーマップをライブ編集できます。
- **RGB インジケータ（`cornix_indicator`）** を有効なターゲットに組み込んでいます。

> ### RMK ファームウェアをお探しですか？
> Cornix の標準ファームウェアは **RMK** です。対応する RMK カスタムビルドはこちらで管理しています。
> **https://github.com/numachang/cornix-rmk-custom**
>
> RMK のまま使いたい場合はそちらを、ZMK を使いたい場合は*このリポジトリ*を利用してください。

> ### キーマップ編集：ZMK Studio Tweaks（カスタム Web エディタ）
> ZMK Studio の改変版を **https://zmk-studio-tweaks.numachang.com/** で公開しています
>（ソース: **https://github.com/numachang/zmk-studio-tweaks**）。Web Serial API を使って Chromium ブラウザ上で
> 完結して動作し（データは手元の PC から外に出ません）、`cornix_left` を開いてそのままキーマップを編集できます。
>
> 公式 ZMK Studio のフォークで、アップストリームのエディタに加えて次の機能を追加しています。
> - **キーマップのインポート/エクスポート**（JSON ファイル）。ファームウェアからのエラーフィードバック付き。
> - **ビジュアルキーピッカー** — 検索付きのグリッド型 HID ピッカー、カテゴリ分けされた behavior、
>   クリックできる物理配列レンダリング（ANSI/ISO/**JIS**）。
> - **ホスト配列のローカライズ** — **日本語**を含む 22 の OS 配列に対応。基盤となる HID バインディングは変えずに、
>   ピッカーとラベルがあなたのキーボード配列に合わせて表示されます。
>
> これはアップストリームのレビューサイクルを待たずに機能を試作する場であり、きれいに収まるものは PR として
> アップストリームへ還元する意図で作っています。ZMK プロジェクトとは**無関係で、公認でもありません**。
> 安定したサポート付きの利用には[公式 ZMK Studio](https://zmk.studio/) を推奨します。

![image](images/cornix_with_dongle.png)
![image](images/cornix_layout.png)

## ボードとシールド

### ボード

- **`cornix_left`**: Cornix 分割の左半分。セントラル（マスター）側として動作します。ここで ZMK Studio が有効です。
- **`cornix_right`**: Cornix 分割の右半分。ペリフェラル（スレーブ）側として動作します。
- **`cornix_ph_left`**: ドングル構成向けの代替左半分ボード。**このフォークでは未使用**です
  （アップストリームとの整合のために残しています）。

### シールド

- **`cornix_indicator`**: バッテリーと接続状態を示す RGB LED インジケータを有効にします。消費電力が増えます。
- **`cornix_dongle_adapter`** / **`cornix_dongle_eyelash`**: アップストリーム由来のドングル関連シールド。
  ディスク上には存在しますが、このフォークの**有効なビルドマトリクスからは参照されていません**。

このコミュニティファームウェアは ZMK を用いた Cornix で動作確認済みで、ZMK の分割ガイドラインに沿って、
完全な分割ロール設定、バッテリー電源管理、Bluetooth セントラル/ペリフェラル設定を提供します。

## 対応ハードウェア：Cornix 分割キーボード

Cornix Split Tented Low-Profile Ergo Keyboard (Jezail Funder)

Cornix は Corne にインスパイアされた分割エルゴノミクスキーボードで、コンパクトな 3×6 カラムスタッガード配列と
6 個の親指クラスターキー（片手 3 個）を備えています。10°・18°・25° のチルト（テンティング）角度を調整でき、
手首への負担を減らして自分に合ったエルゴノミクスな角度を見つけられます。

- **分割・カラムスタッガード配列**（3×6 + 親指クラスター）。
- **チルト角度の調整**：10°・18°・25°（ハードウェアベースで、ファームウェアの細工は不要）。
- **Kailh Choc V2 ホットスワップソケット**、LAK / LCK ロープロファイルキーキャップに対応。
- **デュアルモード接続**：有線 USB-C または Bluetooth ワイヤレス（左半分がセントラル）。
- **ファームウェア**：標準は RMK（VIAL 対応）。このリポジトリは ZMK の代替を提供します。
- 高品質な **CNC 切削アルミシャーシ**、専用の制振フォーム、持ち運び用ポーチ付き。

> アップストリームのプロジェクトオーナーは RMK のコントリビューターでもあります。RMK もぜひご支援ください
> https://rmk.rs/

## ブートローダー / リカバリーについて

ボードの **v2.3** でフラッシュのパーティションレイアウトが更新され、SoftDevice 用パーティションが縮小されました
（150K → 4K）。そのため `.uf2` を直接書き込めます。**SoftDevice の復元は不要**です。

- 非常に古いファームウェアから移行する場合は、先に `reset.uf2` でリセットが必要なことがあります。
- 標準の RMK ファームウェアへは、元の `.uf2` を書き込めばロールバックできます。バックアップは `rmkfw/` にあります。
- より踏み込んだブートローダーリカバリー手順（旧来の SoftDevice 復元方式）は
  [bootloader/README.md](./bootloader/README.md) を参照してください。

## 🔰 かんたんな方法：このリポジトリを clone して GitHub Actions でビルド

ZMK が初めてで `west.yml` やモジュール管理に触れたくない場合は、このリポジトリをそのまま使ってファームウェアを
カスタマイズできます。

### 手順

1. **このリポジトリを Fork または clone する**
   - 右上の **Fork** で自分の GitHub アカウントへコピーするか、ローカルで `git clone` します。

   > Fork がおすすめです。GitHub Actions が自動でファームウェアをビルドしてくれます。

2. **キーマップを編集する**
   - キーマップは `config/cornix.keymap` にあります。直接編集するか、ZMK Studio（`cornix_left` で有効）—
     [公式エディタ](https://zmk.studio/) または改変版
     [zmk-studio-tweaks.numachang.com](https://zmk-studio-tweaks.numachang.com/)（上記参照）— あるいは
     [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) を使います。
   - 変更を commit して GitHub に push します。

3. **GitHub Actions でビルドする**
   - `boards/*` または `config/*` 配下の変更を push すると自動でビルドが走ります。**Actions →
     「Build ZMK firmware」→ Run workflow** から手動実行もできます（`build.yaml` だけの変更では自動ビルドは
     走らないので、その場合は手動実行してください）。
   - ワークフロー完了後、**Actions → 最新の実行 → Artifacts** からファームウェアをダウンロードします
     （`firmware.zip`。ターゲットごとに 1 つの `.uf2` が入っています）。

4. **キーボードに書き込む**
   - 各半分を UF2 ブートローダーモードにします（通常はリセットボタンのダブルタップ）。
   - 対応する `.uf2` をマウントされたドライブにドラッグ＆ドロップします。
   - **データ通信対応の USB-C ケーブル**を使い、**両半分を同じビルドから**書き込んでください。
     分割プロトコルの不一致を避けるためです。

### こんな人向け

- ZMK を始めたばかりの人
- キーマップだけカスタマイズしたい人
- ドライバやハードウェア定義を変更する必要がない人

## ビルドターゲット（`build.yaml`）

このフォークでは、次の有効なターゲットを用意しています（ドングルターゲットはコメントアウト済み）。

```yaml
include:
  # 左半分（セントラル）— RGB インジケータ + USB 経由の ZMK Studio
  - board: cornix_left
    shield: cornix_indicator
    snippet: studio-rpc-usb-uart nrf52840-nosd
    artifact-name: cornix_left_default_nosd

  # 右半分（ペリフェラル）— RGB インジケータ
  - board: cornix_right
    shield: cornix_indicator
    artifact-name: cornix_right_nosd

  # 設定リセット（再ペアリング時に両半分へ書き込む）
  - board: cornix_right
    shield: settings_reset
    snippet: studio-rpc-usb-uart nrf52840-nosd
    artifact-name: cornix_reset
```

> [!NOTE]
> - `cornix_indicator` シールドは RGB LED を有効にしますが、消費電力が増えます。インジケータが不要なら、
>   そのターゲットから外してください。
> - 再ペアリングの手順：両側に `cornix_reset` を書き込み、その後 `cornix_left` と `cornix_right` を書き込んで、
>   両側を同時にリセットします。

## 既存の ZMK プロジェクトに Cornix モジュールを追加する

既存の zmk-config がある場合は、このリポジトリを `west.yml` の依存として追加し、`west update` で取得します。

### 1. `west.yml` を修正する

`manifest/remotes` セクションに追加：

```yaml
remotes:
  - name: zmkfirmware
    url-base: https://github.com/zmkfirmware
  - name: numachang
    url-base: https://github.com/numachang
  - name: urob
    url-base: https://github.com/urob
```

`manifest/projects` セクションに追加：

```yaml
projects:
  - name: zmk
    remote: zmkfirmware
    revision: main
    import: app/west.yml
  - name: cornix-zmk-custom
    remote: numachang
    revision: main
  - name: zmk-helpers
    remote: urob
    revision: main
```

> 日本向けフォークではなくアップストリームを使う場合は、`url-base: https://github.com/hitsmaxft`、
> プロジェクト名 `zmk-keyboard-cornix` を指定してください。

### 2. 依存を更新する

```bash
west update
```

### 3. ビルドを設定する

[ビルドターゲット](#ビルドターゲットbuildyaml) のターゲットを `build.yaml` に追加します。

### 4. ビルドして書き込む

- SoftDevice の復元は不要です（ボード v2.3 以降）。
- 再ペアリングするなら両側に `cornix_reset` を書き込みます。
- 左右の `.uf2` を書き込み、両側を同時にリセットします。

## このプロジェクトをローカルでビルドする（`west.yml` 依存なし）

`west.yml` の依存に追加せずローカルでビルドしたい場合は、`ZMK_EXTRA_MODULES` の cmake 引数を使えます。

### 前提

1. 動作する ZMK 開発環境。
2. このリポジトリをローカルディレクトリに clone 済みであること。

### 手順

1. **このリポジトリを clone する**：
   ```bash
   git clone https://github.com/numachang/cornix-zmk-custom.git
   ```

2. **追加モジュールとして ZMK ビルドを設定する** — `.west/config` を編集し、`[build]` セクションに cmake 引数を
   追加します：

   ```ini
   [build]
   cmake-args = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DZMK_EXTRA_MODULES=/full/absolute/path/to/cornix-zmk-custom
   ```

   パスは clone した実際の絶対パスに置き換えてください。

3. **ファームウェアをビルドする**：
   ```bash
   west build -b cornix_left
   west build -b cornix_right
   ```

この方法なら、既存の ZMK 設定の `west.yml` を変更せずに Cornix モジュールを使えます。

### Nix を使ったローカルビルド（Linux / macOS）

`Justfile` 経由で Nix ベースのフローを用意しています。

```bash
nix develop          # zephyr-sdk, west, just, yq, keymap-drawer を提供
just init            # west init -l config && west update && west zephyr-export
just list            # build.yaml から解析したビルドターゲットを表示
just build cornix_left
just clean
just draw cornix     # keymap-drawer でキーマップ SVG を描画
```

Windows ユーザーは GitHub Actions を利用してください。

### Docker を使ったローカルビルド（CI と同一環境）

`docker-build.sh` は、GitHub Actions が使っているのと同じコンテナイメージ
（`zmkfirmware/zmk-build-arm:stable`）でビルドします。ツールチェーンが CI とずれないので、
**CI が落ちた原因を手元で再現したいとき**はこちらを使ってください。必要なのは Docker だけです。

```bash
./docker-build.sh init          # 初回のみ: zmk + zephyr + modules を取得（約 3 GB）
./docker-build.sh list          # build.yaml から解析したビルドターゲットを表示
./docker-build.sh build         # 有効な全ターゲットをビルド
./docker-build.sh build left    # 名前が "left" に一致するターゲットだけビルド
./docker-build.sh update        # config/west.yml を変更した後に west update をやり直す
./docker-build.sh shell         # ワークスペース内で対話シェルを開く
./docker-build.sh clean         # ビルド成果物だけ削除（取得済みソースは残す）
```

生成された `.uf2` は `firmware/` に出力されます（`.gitignore` 済み）。

west のワークスペースは**リポジトリの外**（既定で `~/zmk-workspace-cornix`、`ZMK_WORKSPACE` で変更可）
に作られます。本リポジトリは自身の `zephyr/module.yml` を持つため、リポジトリ直下をワークスペースにすると
`west update` が `<topdir>/zephyr` に clone する Zephyr ツリーと衝突するからです。CI も
`zephyr/module.yml` がある場合は `$TMPDIR/zmk-config` を使って同じ回避をしています。

## RGB について

Cornix シールドは片側に 2 個の RGB LED を備え、標準ファームウェアでは PWM で制御されています。ZMK では
`cornix_indicator` シールドがこれらを駆動し、バッテリーと接続状態を表示します。完全にネイティブな
ツリー外 RGB モジュールも計画中です（issue #2 参照）。
