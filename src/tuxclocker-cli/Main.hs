{-# LANGUAGE OverloadedStrings #-}

import Data.Either
import Data.Functor
import Data.Maybe
import Data.Text (Text)
import DBus
import DBus.Client
import qualified DBus.Introspection as I

tuxClockerCall :: MethodCall -> MethodCall
tuxClockerCall call = call { methodCallDestination = Just "org.tuxclocker" }

getObject :: Client -> ObjectPath -> IO I.Object
getObject client path = do
  reply <- call_ client $
    tuxClockerCall $ methodCall path "org.freedesktop.DBus.Introspectable" "Introspect"
  let xml = fromVariant (head $ methodReturnBody reply)
  pure $ fromMaybe
    (error ("Invalid introspection XML: " ++ show xml))
    (xml >>= I.parseXML path)

getName :: Client -> ObjectPath -> IO Variant
getName client path =
  let
    call = tuxClockerCall $ methodCall path "org.tuxclocker.Node" "name"
  in
    getProperty client call <&> fromRight (toVariant ("Unnamed" :: Text))

printDBusTree :: Client -> IO ()
printDBusTree client =
  go client "/" where
    go client path = do
      object <- getObject client path
      print $ I.objectPath object
      name <- getName client $ I.objectPath object
      print name
      mapM_ (go client) (I.objectPath <$> I.objectChildren object)

main = do
  client <- connectSystem
  printDBusTree client
