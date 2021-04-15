# IPC流用可能部

## 概要

* ServerとClientの通信処理(IPC)部分を汎用的に実装したものです。
* 大きく以下で構成されています。
  * IPCライブラリ実装ソース: src, include
  * IPC単体テスト用プログラム: ipc_unit_test

# ビルド方法

* 以下の手順でビルド可能です。
  ```bash
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
  ```
  * 上記のコマンドは以下のスクリプトでも実行可能です。  
    ```bash
    $ ./buildtest.sh
    ```

後述するインストールでは、ホストPC(/usr/local/)にインストールされる。インストール先を変更するためには、cmakeに渡すオプションを変更する。

```
例
$ cmake -DCMAKE_INSTALL_PREFIX=./install ..
```

# インストール方法

  ```bash
  $ cd build
  $ sudo make install
  ```

成功すると、下記のログが出力される。

```
[100%] Built target ipc
Install the project...
-- Install configuration: ""
-- Installing: /usr/local/lib/pkgconfig/ipc.pc
-- Installing: /usr/local/lib/libipc.so.1.0.0
-- Installing: /usr/local/lib/libipc.so.1
-- Installing: /usr/local/lib/libipc.so
-- Installing: /usr/local/include/ipc.h
-- Installing: /usr/local/include/ipc_protocol.h
```

# ビルド生成物

* ビルドにより、最終的には以下が生成されます。  
  * \<installdir\>/include/ 以下
    外部公開向けヘッダファイル  
    ```bash
    ipc.h
    ipc_protocol.h
    ```
  * \<installdir\>/lib/ 以下  
    共有ライブラリファイル  
    ```bash
    libipc.so   (シンボリックリンク)
    libipc.so.1 (シンボリックリンク)
    libipc.so.1.0.0
    ```
  * build/ipc_unit_test/ 以下  
    テストプログラム実行ファイル  
    ```bash
    ipc_unit_test_client
    ipc_unit_test_server
    ```
<br>

# 使用方法

* 本ライブラリにはServer用とClient用の機能が入っており、それぞれ使用方法の異なる部分があります。

## Server/Client共通

* 使用者は以下のライブラリとリンクする必要があります。
  * `libipc.so`
* 本ライブラリ使用者は以下をincludeします。
  * #include <ipc.h>
    * 後述のipc_protocol.hはipc.h内からincludeされるため、記載不要です。
* 使用するヘッダファイルは以下の通りです。
  * ipc.h
    * 使用可能なAPI関数一覧が宣言されています。
  * ipc_protocol.h
    * IPCの用途種別と、各用途に応じたデータ構造体が定義されています。
* 本ライブラリでは、Unix Domain Socketを用いてServerとClientが通信します。
  * 用途種別ごとに異なる通信用ファイルを生成します。
  * デフォルトではServerの実行階層に通信用ファイルを生成します。
  * 環境変数 "IPC_DOMAIN_PATH" を設定しておくことで、通信用ファイルの生成場所を変更することができます。
  ```
  例)
  $ export IPC_DOMAIN_PATH="/tmp"
    →/tmp以下にUnix Domain Socket通信用ファイルが生成されるようになる。
  ```

## IC-Service向け

* 本ライブラリをIC-Service向けに使用する場合、以下の値・構造体を用います(ipc_protocol.h参照)。
  * 用途種別usageType：IPC_USAGE_TYPE_IC_SERVICE
  * 送信データ構造体：IPC_DATA_IC_SERVICE_S
  * 変化種別コールバック通知用enum：IPC_KIND_IC_SERVICE_E
  * Unix Domain Socket通信用ファイル名：IpcIcService
* IC-Serviceの場合、Cluster APIライブラリ(libcluster_api.so)がIPC Clientとなっています。
  * 後述のClient用 APIは、libcluster_api.so内から呼び出されます。

## Server用 API

* libipc.soを用いるServerは以下のAPIを使用できます。
  * ipcServerStart(IPC_USAGE_TYPE_E usageType);
    * 指定した用途種別usageType用のIPC Serverを起動します。
  * ipcSendMessage(IPC_USAGE_TYPE_E usageType, const void* pData, signed int size);
    * 指定した用途種別usageType用に、IPC Clientへのデータ送信を行います。
    * 送信データのアドレスとサイズを引数pData, sizeで指定します。
    * 送信データは、IPC Client側で用意しているData Poolに格納されます。
  * ipcServerStop(IPC_USAGE_TYPE_E usageType);
    * 指定した用途種別usageType用のIPC Serverを終了します。

## Client用 API

* libipc.soを用いるClientは以下のAPIを使用できます。
  * ipcClientStart(IPC_USAGE_TYPE_E usageType);
    * 指定した用途種別usageType用のIPC Clientを起動します。
    * 同じusageType用のIPC Serverと接続します。
  * ipcReadDataPool(IPC_USAGE_TYPE_E usageType, void* pData, signed int* pSize);
    * 指定したusageType用のData Poolの全データを読み込みます。
    * 読み込みデータ格納先のアドレスはpDataに、格納可能なサイズはpSizeに指定します。
    * Data Poolの内容はpDataに出力され、実際に読み込めたサイズはpSizeに出力されます。
  * ipcRegisterCallback(IPC_USAGE_TYPE_E usageType, IPC_CHANGE_NOTIFY_CB changeNotifyCb);
    * IPC Serverからデータを受信した時、どのデータが何に変化したかの通知を受けるためのコールバック関数を、指定したusageType用に登録します。
  * ipcClientStop(IPC_USAGE_TYPE_E usageType);
    * 指定した用途種別usageType用のIPC Clientを終了します。

# 単体テスト実行方法

* 制限  
  * 2020/12/25現在、このテストプログラムは必ずIC-Service用としてServerとClientを接続します。  
  * unix domain socket通信用ファイルは /tmp/ 以下に ipcIcService というファイル名で生成されます。

* ipc_unit_test_server と ipc_unit_test_client 間でプロセス間通信を行いますので、それぞれを別々のターミナルで起動します。  
  操作は手動ですが、以下、テストの一例です。

  1. **Server, Clientの順に起動します。**  
      ```bash
      (Terminal 1)
      $ ./ipc_unit_test_server
      command (h=help, q=quit):
      ```
      ```bash
      (Terminal 2)
      $ ./ipc_unit_test_client
      command (h=help, q=quit):
      ```  
      この時点でServerとClientはIC-Service用として接続済みです。  
      (ipcServerStart()とipcClientStart()が実行されています)  

  2. **Server側の送信データを適当に編集して送信してみます。**  
      ```bash
      (Terminal 1)
      command (h=help, q=quit):w ←★ w を入力
      write command (h=help q=goto main menu):2 1 ←★2 1を入力
      write command (h=help q=goto main menu):70 50 ←★70 50を入力
      write command (h=help q=goto main menu):l ←★l を入力
      ★送信データ一覧が表示されるが、以下、入力内容が反映されていること。
        2: brake(4) = 1 ←★write command 2 1 の結果
        70: oTempUnitVal(4) = 50 ←write command 70 50 の結果
      write command (h=help q=goto main menu):q ←★q を入力
      command (h=help, q=quit):s ←★s を入力(ipcSendMessage()の実行)
      ipcSendMessage return:0
      command (h=help, q=quit):
      ```
      Client側にコールバック関数の反応が出ているはず。
      ```bash
      (Terminal 2)
      command (h=help, q=quit):Enter changeNotifyCb ←★コールバック
      kind = 2, size = 4, data=1 ←★brakeの値が1に変わったことを通知
      Leave changeNotifyCb
      ```
      ★oTempUnitValの変化についてはIC-Serviceとしては監視対象でないのでコールバック無し。

  3. **Client側で受信できているか確認します。**
      ```bash
      (Terminal 2)
      command (h=help, q=quit):r ←★r を入力
      ★送信データ一覧が表示されるが、以下、送信されたデータが入っていること。
        2: brake(4) = 1
       70: oTempUnitVal(4) = 50
      ```

  4. **Client, Serverの順に終了します。**
      ```bash
      (Terminal 2)
      command (h=help, q=quit):q
      bye...
      $
      ```
      ```bash
      (Terminal 1)
      command (h=help, q=quit):q
      bye...
      $
      ```

# IPC用途種別の追加・変更方法

* まずはIC-Service向けにのみ実装しましたが、別の用途向けのデータを容易に追加することが可能なように構成しています。
* 各用途種別向けの情報は、以下のファイルにて管理しています。
  * include/ipc_protocol.h (外部公開ヘッダ)
  * src/ipc_usage_info_table.c (IPC内部向けソース)
* 新規用途の情報追加、もしくは既存用途への情報変更は上記2つのファイルに対してのみ行うだけで良いようにしています。
  * ipc内の他の.cファイルや.hファイルに対しては変更不要です。
  * ただし、その用途でIPCを用いるアプリやテストプログラムに対しては、ipc_protocol.hへの定義追加・変更に合わせた対応が別途必要になります。
* 理想ではツール等でコードを自動生成できることが理想ですが、今回はそこまでの実装を考慮しておりません。

## 用途種別の追加・変更に関するサンプルコード（差分）

まずは2つのファイルに対してどのように追加・変更する際のサンプルコードを示します。

### 例1：新規の用途種別を追加する場合

仮にNEW_SERVICEという用途種別を追加する場合の差分例を示します。
IPC内における、新規の用途種別追加による影響範囲となります。

```patch
diff --git a/include/ipc_protocol.h b/include/ipc_protocol.h
index c0ad861..2bc1115 100644
--- a/include/ipc_protocol.h
+++ b/include/ipc_protocol.h
@@ -6,6 +6,7 @@
 typedef enum {
     IPC_USAGE_TYPE_IC_SERVICE = 0,
     IPC_USAGE_TYPE_FOR_TEST,
+    IPC_USAGE_TYPE_NEW_SERVICE, // 用途種別追加
     IPC_USAGE_TYPE_MAX
 } IPC_USAGE_TYPE_E;

@@ -145,4 +146,17 @@ typedef struct {
     signed int test;
 } IPC_DATA_FOR_TEST_S;

+// for IPC_USAGE_TYPE_NEW_SERVICE
+typedef enum { // データ変化を監視・通知したい種別のみ用意
+    IPC_KIND_NS_PARAM1 = 0,
+    IPC_KIND_NS_PARAM2
+} IPC_KIND_NEW_SERVICE_E;
+
+typedef struct { // この用途で送受信する全データ
+    int param1;
+    int param2;
+    int param3;
+    int param4;
+} IPC_DATA_NEW_SERVICE_S;
+
 #endif // IPC_PROTOCOL_H
diff --git a/src/ipc_usage_info_table.c b/src/ipc_usage_info_table.c
index 976cc73..51264c6 100644
--- a/src/ipc_usage_info_table.c
+++ b/src/ipc_usage_info_table.c
@@ -51,16 +51,24 @@ static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeForTest[] = {
     DEFINE_OFFSET_SIZE(IPC_DATA_FOR_TEST_S, test, IPC_KIND_TEST_TEST)
 };

+//   for IPC_USAGE_TYPE_FOR_TEST
+static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeNewService[] = { // データ変化を監視・通知したい種別のみ記載
+    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param1, IPC_KIND_NS_PARAM1),
+    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param2, IPC_KIND_NS_PARAM2)
+}; // この例では、param3, param4のデータ変化については監視・通知しない。
+
 // == usage info table ==
 //   index of [] is IPC_USAGE_TYPE_E
 IPC_DOMAIN_INFO_S g_ipcDomainInfoList[] =
 {
     {sizeof(IPC_DATA_IC_SERVICE_S), "ipcIcService"},
-    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"}
+    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"},
+    {sizeof(IPC_DATA_NEW_SERVICE_S), "ipcNewService"} // 新規用途用の送受信サイズ情報追加
 };

 IPC_CHECK_CHANGE_INFO_TABLE_S g_ipcCheckChangeInfoTbl[] = {
     DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeIcService),
-    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest)
+    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest),
+    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeNewService) // 新規用途用 データ変化監視テーブルを登録
 };

```

### 例2：既存の用途種別のデータの一部を削除する場合

既存の用途IC-Serviceの送受信データから、メンバ変数brakeを削除する場合の差分例を示します。
IPC内における、メンバ変数を削除した場合の影響範囲となります。

```patch
diff --git a/include/ipc_protocol.h b/include/ipc_protocol.h
index c0ad861..7fed8bf 100644
--- a/include/ipc_protocol.h
+++ b/include/ipc_protocol.h
@@ -13,7 +13,6 @@ typedef enum {
 typedef enum {
     IPC_KIND_ICS_TURN_R = 0,
     IPC_KIND_ICS_TURN_L,
-    IPC_KIND_ICS_BRAKE,
     IPC_KIND_ICS_SEATBELT,
     IPC_KIND_ICS_HIGHBEAM,
     IPC_KIND_ICS_DOOR,
@@ -51,7 +50,6 @@ typedef struct {
     // Telltale
     signed int turnR;
     signed int turnL;
-    signed int brake;
     signed int seatbelt;
     signed int frontRightSeatbelt;
     signed int frontCenterSeatbelt;
diff --git a/src/ipc_usage_info_table.c b/src/ipc_usage_info_table.c
index 976cc73..40ac8df 100644
--- a/src/ipc_usage_info_table.c
+++ b/src/ipc_usage_info_table.c
@@ -12,7 +12,6 @@
 static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeIcService[] = {
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, turnR, IPC_KIND_ICS_TURN_R),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, turnL, IPC_KIND_ICS_TURN_L),
-    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, brake, IPC_KIND_ICS_BRAKE),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, seatbelt, IPC_KIND_ICS_SEATBELT),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, highbeam, IPC_KIND_ICS_HIGHBEAM),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, door, IPC_KIND_ICS_DOOR),
```

## 用途種別の新規追加に関する共通事項

* いくつか列挙体・構造体の新規追加、および既存の列挙体・構造体内への追記を伴いますが、いずれも名称については特に制約はありません。

## include/ipc_protocol.h へ追記する情報
* 1つの用途種別に対し、以下の3つの情報を追記します。
  * 用途種別名の追記
  * 新規用途向けの変化通知種別用列挙体の定義
  * 新規用途向けの送受信データ構造体の定義
* 用途種別名の追記
  * サンプルコードの以下の部分のことになります。
    ```patch
     typedef enum {
         IPC_USAGE_TYPE_IC_SERVICE = 0,
         IPC_USAGE_TYPE_FOR_TEST,
    +    IPC_USAGE_TYPE_NEW_SERVICE, // 用途種別追加
         IPC_USAGE_TYPE_MAX
     } IPC_USAGE_TYPE_E;
    ```
  * enum IPC_USAGE_TYPE_E 内に用途種別となるメンバを追加します。
  * IPC_USAGE_TYPE_MAXの1つ手前に追加するようにしてください(既存の定義に影響を及ぼさないようにするため)。
  * ここで定義した値は、ipc.hで定義されているipcServerStart()などの引数usageTypeへの指定用に使用します。
* 新規用途向けの変化通知種別用列挙体の定義
  * サンプルコードの以下の部分のことになります。
    ```patch
    +typedef enum { // データ変化を監視したい種別のみ用意
    +    IPC_KIND_NS_PARAM1 = 0,
    +    IPC_KIND_NS_PARAM2
    +} IPC_KIND_NEW_SERVICE_E;
    ```
  * データ変化通知の種別用列挙体を追加します。後述の送受信データ構造体と関連があります。
  * この値は、ipcRegisterCallback()で登録されたコールバック関数の第3引数kindへの指定に使用します。
  * 列挙体名、メンバ名について、特に名称の制約はありません。
* 新規用途向けのデータ構造体の定義
  * サンプルコードの以下の部分のことになります。
    ```patch
    +typedef struct { // この用途で送受信する全データ
    +    int param1;
    +    int param2;
    +    int param3;
    +    int param4;
    +} IPC_DATA_NEW_SERVICE_S;
    ```
  * 新規用途で送受信するデータ構造体を追加します。
  * IPC ServerからIPC Clientへは、ここで定義した構造体のデータ全てを送信することになります。

## src/ipc_usage_info_table.c に対する追記
* 1つの用途種別に対し、以下の3つの情報を追記します。
  * データ変化通知用の種別対応テーブルの追加
  * 通信用ドメイン情報追記(通信サイズ、ドメインファイル名)
  * 用途と変化種別対応テーブルとの関係追記
* データ変化通知用の種別対応テーブルの追加
  * サンプルコードの以下の部分のことになります。
    ```
    +//   for IPC_USAGE_TYPE_FOR_TEST
    +static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeNewService[] = { // データ変化を監視・通知したい種別のみ記載
    +    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param1, IPC_KIND_NS_PARAM1),
    +    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param2, IPC_KIND_NS_PARAM2)
    +}; // この例では、param3, param4のデータ変化については監視・通知しない。
    ```
  * 新規用途向けに、IPC_CHECK_CHANGE_INFO_Sの構造体配列を追加します。
  * ipc_protocol.hで定義した変化通知種別用列挙体の定義と、通信するデータ構造体メンバを対応付けるテーブルを記載します。
  * このテーブルは、IPC ClientがIPC Serverからデータを受信する時に、前回受信時と変化しているデータ種別をコールバック通知する際に使用します。
  * 構造体配列内には、以下のようなマクロを複数個記載して対応を定義します。
    ```c
    DEFINE_OFFSET_SIZE(<データ構造体名>, <構造体メンバ名>, 変化通知列挙体メンバ名),
    ```
  * 上記サンプルコード、g_ipcCheckChangeNewService[]の場合は以下のようになります。
    * param1が前回受信時と値が異なる場合、変化種別 IPC_KIND_NS_PARAM1 としてIPC Clientへコールバック通知する。
    * param2が前回受信時と値が異なる場合、変化種別 IPC_KIND_NS_PARAM2 としてIPC Clientへコールバック通知する。
    * 記載していないparam3, param4については、前回受信時と値が異なっていてもコールバック通知はしない。

* 通信用ドメイン情報追記(通信サイズ、ドメインファイル名)
  * サンプルコードの以下の部分のことになります。
    ```patch
     IPC_DOMAIN_INFO_S g_ipcDomainInfoList[] =
     {
         {sizeof(IPC_DATA_IC_SERVICE_S), "ipcIcService"},
    -    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"}
    +    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"},
    +    {sizeof(IPC_DATA_NEW_SERVICE_S), "ipcNewService"} // 新規用途用の送受信サイズ情報追加
     };
    ```
  * 構造体配列 g_ipcDomainInfoList[] に、新規用途向けのドメイン情報を追記します。
  * この追記により、新規追加した用途種別で用いる送受信データサイズと、Unix Domain Socket通信で用いるドメインファイル名が決まります。
  * ipc_protocol.hのenum IPC_USAGE_TYPE_Eの定義順と一致させる必要があるので、必ず末尾に追加してください。
  * 以下のように、通信するデータ構造体のサイズと、ドメインファイル名の情報を、g_ipcDomainInfoList[] の末尾に追記します。
    ```c
    {sizeof(<通信するデータ構造体名>), "ドメインファイル名"},
    ```
* 用途と変化種別対応テーブルとの関係追記
  * サンプルコードの以下の部分のことになります。
    ```patch
     IPC_CHECK_CHANGE_INFO_TABLE_S g_ipcCheckChangeInfoTbl[] = {
         DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeIcService),
    -    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest)
    +    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest),
    +    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeNewService) // 新規用途用 データ変化監視テーブルを登録
     };
    ```
  * 構造体配列 g_ipcCheckChangeInfoTbl[] に、新規用途向けの変化通知種別対応テーブルに関する情報を追記します。
  * ipc_protocol.hのenum IPC_USAGE_TYPE_Eの定義順と一致させる必要があるので、必ず末尾に追加してください。
  * 前述の「変化通知種別対応テーブル構造体」を、以下のようなマクロに記載し、g_ipcCheckChangeInfoTbl[]の末尾に追記します。
    ```c
    DEFINE_CHANGE_INFO_TABLE(<変化通知種別対応テーブル構造体名>),
    ```

## 既存用途向けの送信データを一部変更する場合
* ipc_protocol.h内の既存の送信データ構造体内のメンバ変数の削除、もしくは名称変更する場合
  * ipc部分、およびipcをその用途で用いるアプリをそれぞれビルドしてみて、コンパイルエラーとなった部分を修正します。

* ipc_protocol.h内の既存の送信データ構想体へメンバ変数を追加する場合
  * [IPC用途種別の追加・変更方法](#IPC用途種別の追加・変更方法) を参考に、include/ipc_protocol.hとsrc/ipc_usage_info_table.cへの追記を行います。

## 補足
* src/ipc_usage_info_table.cにて、DEFINE_OFFSET_SIZE()マクロにて情報を記載しているが、これはoffsetof()とsizeof()を使うことで、メンバ変数に関する構造体先頭からオフセットとサイズを取得しています。
  * 用途種別の追加を容易に行えるようにするため、IPC処理内部ではデータ構造体内の変数名を直接指定しないような実装を行っています。
  * 各用途に対して、データ構造体のオフセットテーブルを用意することで、送信されたデータの何バイト目に何の変数があるかがわかるようになります。
    * この仕組みにより、IPC処理内部でメンバ変数名を直接指定しなくとも、データ変化の確認が可能となります。
  * [IPC用途種別の追加・変更方法](#IPC用途種別の追加・変更方法) に従って用途種別を追加することで、IPC内部処理は新たな用途に対する処理ができるようになります。
