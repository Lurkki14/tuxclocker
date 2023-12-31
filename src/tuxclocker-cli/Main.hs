{-# LANGUAGE OverloadedStrings #-}

import Data.Either
import Data.Functor
import Data.Maybe
import Data.Text (Text)
import Data.Tree
import DBus
import DBus.Client
import qualified DBus.Introspection as I

tuxClockerCall :: MethodCall -> MethodCall
tuxClockerCall call = call { methodCallDestination = Just "org.tuxclocker" }

printTree :: Show a => Tree a -> IO ()
printTree tree = printIndent tree 0 where
  printIndent :: Show a => Tree a -> Int -> IO ()
  printIndent (Node rootLabel subForest) level = do
    print $ (replicate level ' ') <> show rootLabel
    mapM_ (\tree -> printIndent tree (level + 1)) subForest

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

getDBusTree :: Client -> IO (Tree Variant)
getDBusTree client = unfoldTreeM (buildNode client) "/" where
  buildNode :: Client -> ObjectPath -> IO (Variant, [ObjectPath])
  buildNode client path = do
    object <- getObject client path
    name <- getName client $ I.objectPath object
    let childPaths = I.objectPath <$> I.objectChildren object
    pure (name, childPaths)

main = do
  client <- connectSystem
  getDBusTree client >>= printTree
