open Log;

type config = {
  host: string,
  port: int
};

let decodeConfig = json =>
  Json.Decode.{
    host: json |> field("host", string),
    port: json |> field("port", int)
  };

[@bs.deriving abstract]
type systemError = {
  [@bs.optional] code: string
};
external toSystemError: Js.Exn.t => systemError = "%identity";

let defaultConfig = {|{
  "host": "127.0.0.1",
  "port": 6700
}
|};

let config = try(Node.Fs.readFileAsUtf8Sync("config.json")) {
    | Js.Exn.Error(e) => {
        switch(toSystemError(e)->codeGet) {
          | Some("ENOENT") => {
            logDebug("Cannot find config file", None);
            try (Node.Fs.writeFileAsUtf8Sync("config.json", defaultConfig)) {
              | Js.Exn.Error(e) => logWarn({j|Fail to create config file|j}, Some(e))
            }
          }
          | Some(_) | None => logWarn({j|Fail to open config file|j}, Some(e))
        };
        defaultConfig
      }
  }
  |> Json.parse
  |> result => {
    switch (result) {
      | Some(value) => value
      | None => {
        logWarn("Fail to parse config file", None);
        defaultConfig->Json.parseOrRaise
      }
    }
  }
  |> decodeConfig;
